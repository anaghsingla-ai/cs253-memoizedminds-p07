#include "cs253/sqrt_decomposition.hpp"
#include "cs253/sqrt_config.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace cs253 {

namespace {

using Wide = __int128_t;

std::size_t configured_block_size = 0;

Residue norm(Wide x){
    x%=modulus;
    if(x<0)x+=modulus;
    return static_cast<Residue>(x);
}

Value checked(Wide x){
    if(x<std::numeric_limits<Value>::min()||
       x>std::numeric_limits<Value>::max()){
        throw std::overflow_error("exact affine arithmetic overflow");
    }
    return static_cast<Value>(x);
}

AffineTag identity_tag(){
    return {1,0};
}

bool is_identity(AffineTag t){
    return t.a==1&&t.b==0;
}

AffineTag compose(AffineTag newer,AffineTag older,Mode mode){
    if(mode==Mode::modular_sum){
        return {
            norm(Wide(newer.a)*older.a),
            norm(Wide(newer.a)*older.b+newer.b)
        };
    }

    return {
        checked(Wide(newer.a)*older.a),
        checked(Wide(newer.a)*older.b+newer.b)
    };
}

Value apply_exact(Value x,AffineTag t){
    return checked(Wide(t.a)*x+t.b);
}

Residue apply_modular(Value x,AffineTag t){
    return norm(Wide(t.a)*x+t.b);
}

Aggregate merge_aggregates(const Aggregate& x,const Aggregate& y){
    if(x.length==0)return y;
    if(y.length==0)return x;

    Aggregate out;
    out.length=x.length+y.length;
    out.sum_mod=norm(Wide(x.sum_mod)+y.sum_mod);

    if(x.minimum&&y.minimum){
        out.minimum=std::min(*x.minimum,*y.minimum);
        out.maximum=std::max(*x.maximum,*y.maximum);
    }

    return out;
}

}

void set_sqrt_block_size(std::size_t block_size) noexcept{
    configured_block_size=block_size;
}

std::size_t sqrt_block_size_setting() noexcept{
    return configured_block_size;
}

struct SqrtDecomposition::Impl{

    struct Block{
        std::size_t begin=0;
        std::size_t end=0;

        AffineTag lazy{1,0};

        Residue sum=0;

        Value mn=0;
        Value mx=0;
    };

    std::vector<Value> raw;
    std::vector<Block> blocks;

    std::size_t n=0;
    std::size_t block_size=1;

    Mode current_mode=Mode::exact_extrema;

    bool built=false;

    Counters stats{};

    void require_built() const{
        if(!built){
            throw std::logic_error(
                "SqrtDecomposition: build() has not been called"
            );
        }
    }

    void validate_range(Range r) const{
        if(r.begin>r.end||r.end>n){
            throw std::out_of_range("invalid range");
        }
    }

    std::size_t block_index(std::size_t i) const{
        return i/block_size;
    }

    Value logical_value(std::size_t i) const{
        const Block& block=blocks[block_index(i)];

        if(current_mode==Mode::modular_sum){
            return apply_modular(raw[i],block.lazy);
        }

        return apply_exact(raw[i],block.lazy);
    }

    void recompute_block(std::size_t k){
        Block& block=blocks[k];

        block.sum=0;

        if(current_mode==Mode::exact_extrema){
            block.mn=logical_value(block.begin);
            block.mx=block.mn;
        }

        for(std::size_t i=block.begin;i<block.end;++i){
            if(current_mode==Mode::modular_sum){
                block.sum=norm(
                    Wide(block.sum)+logical_value(i)
                );
            }
            else{
                Value x=logical_value(i);

                block.sum=norm(
                    Wide(block.sum)+norm(x)
                );

                block.mn=std::min(block.mn,x);
                block.mx=std::max(block.mx,x);
            }
        }
    }

    void push_tag(std::size_t k){
        Block& block=blocks[k];

        if(is_identity(block.lazy)){
            return;
        }

        AffineTag tag=block.lazy;

        for(std::size_t i=block.begin;i<block.end;++i){
            if(current_mode==Mode::modular_sum){
                raw[i]=apply_modular(raw[i],tag);
            }
            else{
                raw[i]=apply_exact(raw[i],tag);
            }
        }

        block.lazy=identity_tag();
    }

    void full_update(std::size_t k,AffineTag tag){
        Block& block=blocks[k];

        const std::size_t length=block.end-block.begin;

        Residue new_sum;

        if(current_mode==Mode::modular_sum){
            tag.a=norm(tag.a);
            tag.b=norm(tag.b);

            new_sum=norm(
                Wide(tag.a)*block.sum+
                Wide(tag.b)*length
            );

            AffineTag new_lazy=compose(
                tag,block.lazy,current_mode
            );

            block.sum=new_sum;
            block.lazy=new_lazy;
            ++stats.tag_compositions;
            return;
        }

        Value new_mn;
        Value new_mx;

        if(tag.a==1){
            new_mn=checked(Wide(block.mn)+tag.b);
            new_mx=checked(Wide(block.mx)+tag.b);
        }
        else if(tag.a==-1){
            new_mn=checked(-Wide(block.mx)+tag.b);
            new_mx=checked(-Wide(block.mn)+tag.b);
        }
        else{
            new_mn=tag.b;
            new_mx=tag.b;
        }

        new_sum=norm(
            Wide(norm(tag.a))*block.sum+
            Wide(norm(tag.b))*length
        );

        AffineTag new_lazy=compose(
            tag,block.lazy,current_mode
        );

        block.sum=new_sum;
        block.mn=new_mn;
        block.mx=new_mx;
        block.lazy=new_lazy;

        ++stats.tag_compositions;
    }

    void partial_update(
        std::size_t k,
        std::size_t l,
        std::size_t r,
        AffineTag tag
    ){
        Block& block=blocks[k];

        push_tag(k);

        for(std::size_t i=l;i<r;++i){
            if(current_mode==Mode::modular_sum){
                raw[i]=apply_modular(raw[i],tag);
            }
            else{
                raw[i]=apply_exact(raw[i],tag);
            }
        }

        stats.partial_elements+=block.end-block.begin;

        recompute_block(k);
    }

    Aggregate block_aggregate(std::size_t k) const{
        const Block& block=blocks[k];

        Aggregate out;
        out.length=block.end-block.begin;
        out.sum_mod=block.sum;

        if(current_mode==Mode::exact_extrema){
            out.minimum=block.mn;
            out.maximum=block.mx;
        }

        return out;
    }

    Aggregate partial_query(
        std::size_t k,
        std::size_t l,
        std::size_t r
    ){
        Aggregate out;
        out.length=r-l;

        if(out.length==0){
            return out;
        }

        const Block& block=blocks[k];

        Residue sum=0;

        if(current_mode==Mode::exact_extrema){
            Value mn=logical_value(l);
            Value mx=mn;

            for(std::size_t i=l;i<r;++i){
                Value x=logical_value(i);

                sum=norm(Wide(sum)+norm(x));

                mn=std::min(mn,x);
                mx=std::max(mx,x);
            }

            out.sum_mod=sum;
            out.minimum=mn;
            out.maximum=mx;
        }
        else{
            for(std::size_t i=l;i<r;++i){
                sum=norm(
                    Wide(sum)+logical_value(i)
                );
            }

            out.sum_mod=sum;
        }

        stats.partial_elements+=r-l;

        return out;
    }

    void build(
        const std::vector<Value>& initial,
        Mode mode
    ){
        built=false;

        n=initial.size();
        current_mode=mode;
        stats={};

        if(configured_block_size!=0){
            block_size=configured_block_size;
        }
        else{
            block_size=1;

            while(block_size*block_size<n){
                ++block_size;
            }
        }

        if(block_size==0){
            block_size=1;
        }

        raw.resize(n);

        if(current_mode==Mode::modular_sum){
            for(std::size_t i=0;i<n;++i){
                raw[i]=norm(initial[i]);
            }
        }
        else{
            raw=initial;
        }

        blocks.clear();

        for(std::size_t start=0;start<n;start+=block_size){
            Block block;

            block.begin=start;
            block.end=std::min(n,start+block_size);
            block.lazy=identity_tag();

            blocks.push_back(block);
        }

        built=true;

        for(std::size_t k=0;k<blocks.size();++k){
            recompute_block(k);
        }

        stats={};
    }

    std::size_t size() const{
        require_built();
        return n;
    }

    Mode mode() const{
        require_built();
        return current_mode;
    }

    void update(Range range,AffineTag tag){
        require_built();
        validate_range(range);

        if(current_mode==Mode::exact_extrema&&
           tag.a!=-1&&tag.a!=0&&tag.a!=1){
            throw std::invalid_argument(
                "exact_extrema requires gain -1, 0, or 1"
            );
        }

        if(range.begin==range.end){
            return;
        }

        if(current_mode==Mode::modular_sum){
            tag.a=norm(tag.a);
            tag.b=norm(tag.b);
        }

        std::size_t first=block_index(range.begin);
        std::size_t last=block_index(range.end-1);

        for(std::size_t k=first;k<=last;++k){
            Block& block=blocks[k];

            std::size_t l=std::max(range.begin,block.begin);
            std::size_t r=std::min(range.end,block.end);

            if(l==block.begin&&r==block.end){
                full_update(k,tag);
                ++stats.full_blocks;
            }
            else{
                partial_update(k,l,r,tag);
            }
        }
    }

    Aggregate query(Range range){
        require_built();
        validate_range(range);

        if(range.begin==range.end){
            return {};
        }

        Aggregate answer;

        std::size_t first=block_index(range.begin);
        std::size_t last=block_index(range.end-1);

        bool has_answer=false;

        for(std::size_t k=first;k<=last;++k){
            const Block& block=blocks[k];

            std::size_t l=std::max(range.begin,block.begin);
            std::size_t r=std::min(range.end,block.end);

            Aggregate current;

            if(l==block.begin&&r==block.end){
                current=block_aggregate(k);
                ++stats.full_blocks;
            }
            else{
                current=partial_query(k,l,r);
            }

            if(has_answer){
                answer=merge_aggregates(answer,current);
                ++stats.aggregate_merges;
            }
            else{
                answer=current;
                has_answer=true;
            }
        }

        return answer;
    }

    Counters counters() const noexcept{
        return stats;
    }

    void reset_counters() noexcept{
        stats={};
    }

    InvariantReport check_invariants() const{
        InvariantReport report{true,{}};

        if(!built){
            report.ok=false;
            report.violations.push_back(
                "build() has not been called"
            );
            return report;
        }

        if(block_size==0){
            report.ok=false;
            report.violations.push_back(
                "block size is zero"
            );
            return report;
        }

        if(blocks.size()!=
           (n+block_size-1)/block_size){
            report.ok=false;
            report.violations.push_back(
                "block count mismatch"
            );
            return report;
        }

        for(std::size_t k=0;k<blocks.size();++k){
            const Block& block=blocks[k];

            std::size_t expected_begin=k*block_size;
            std::size_t expected_end=
                std::min(n,expected_begin+block_size);

            if(block.begin!=expected_begin||
               block.end!=expected_end){
                report.ok=false;
                report.violations.push_back(
                    "block boundaries mismatch"
                );
                continue;
            }

            if(current_mode==Mode::exact_extrema){
                if(block.lazy.a!=-1&&
                   block.lazy.a!=0&&
                   block.lazy.a!=1){
                    report.ok=false;
                    report.violations.push_back(
                        "invalid exact block gain"
                    );
                }
            }
            else{
                if(block.lazy.a<0||
                   block.lazy.a>=modulus||
                   block.lazy.b<0||
                   block.lazy.b>=modulus){
                    report.ok=false;
                    report.violations.push_back(
                        "noncanonical modular block tag"
                    );
                }
            }

            Residue expected_sum=0;

            Value expected_min=0;
            Value expected_max=0;

            bool first=true;

            for(std::size_t i=block.begin;
                i<block.end;
                ++i){

                Value logical;

                try{
                    if(current_mode==Mode::modular_sum){
                        logical=apply_modular(
                            raw[i],block.lazy
                        );
                    }
                    else{
                        logical=apply_exact(
                            raw[i],block.lazy
                        );
                    }
                }
                catch(const std::exception&){
                    report.ok=false;
                    report.violations.push_back(
                        "logical value overflow"
                    );
                    continue;
                }

                expected_sum=norm(
                    Wide(expected_sum)+norm(logical)
                );

                if(current_mode==Mode::exact_extrema){
                    if(first){
                        expected_min=logical;
                        expected_max=logical;
                        first=false;
                    }
                    else{
                        expected_min=std::min(
                            expected_min,logical
                        );

                        expected_max=std::max(
                            expected_max,logical
                        );
                    }
                }
            }

            if(block.sum!=expected_sum){
                report.ok=false;
                report.violations.push_back(
                    "block sum mismatch"
                );
            }

            if(current_mode==Mode::exact_extrema){
                if(block.mn!=expected_min||
                   block.mx!=expected_max){
                    report.ok=false;
                    report.violations.push_back(
                        "block extrema mismatch"
                    );
                }

                if(block.mn>block.mx){
                    report.ok=false;
                    report.violations.push_back(
                        "minimum exceeds maximum"
                    );
                }
            }
        }

        return report;
    }
};

SqrtDecomposition::SqrtDecomposition()
    :impl_(std::make_unique<Impl>()){}

SqrtDecomposition::~SqrtDecomposition()=default;

void SqrtDecomposition::build(
    const std::vector<Value>& initial,
    Mode mode
){
    impl_->build(initial,mode);
}

std::size_t SqrtDecomposition::size() const{
    return impl_->size();
}

Mode SqrtDecomposition::mode() const{
    return impl_->mode();
}

void SqrtDecomposition::update(
    Range range,
    AffineTag tag
){
    impl_->update(range,tag);
}

Aggregate SqrtDecomposition::query(Range range){
    return impl_->query(range);
}

Counters SqrtDecomposition::counters() const noexcept{
    return impl_->counters();
}

void SqrtDecomposition::reset_counters() noexcept{
    impl_->reset_counters();
}

InvariantReport SqrtDecomposition::check_invariants() const{
    return impl_->check_invariants();
}

}
