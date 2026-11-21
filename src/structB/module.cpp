<<<<<<< HEAD


=======
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)
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

<<<<<<< HEAD
std::size_t g_block_setting = 0;

Residue norm(Wide x) {
    x %= modulus;
    if (x < 0) x += modulus;
    return static_cast<Residue>(x);
}

Value checked(Wide x) {
    if (x < std::numeric_limits<Value>::min() || x > std::numeric_limits<Value>::max())
        throw std::overflow_error("exact affine arithmetic overflow");
    return static_cast<Value>(x);
}

bool is_identity(const AffineTag& t) { return t.a == 1 && t.b == 0; }

bool exact_gain_ok(Value a) { return a == -1 || a == 0 || a == 1; }

}

void set_sqrt_block_size(std::size_t block_size) noexcept { g_block_setting = block_size; }
std::size_t sqrt_block_size_setting() noexcept { return g_block_setting; }

struct SqrtDecomposition::Impl {


    struct Block {
        Value a = 1, b = 0;
        Residue sum = 0;
        Value mn = 0, mx = 0;
    };

    bool built = false;
    Mode md = Mode::exact_extrema;
    std::size_t n = 0, bsize = 1, nblocks = 0;
    std::vector<Value> raw;
    std::vector<Block> blocks;
    Counters cnt;

    bool exact() const { return md == Mode::exact_extrema; }
    std::size_t bstart(std::size_t k) const { return k * bsize; }
    std::size_t bend(std::size_t k) const { return std::min(n, (k + 1) * bsize); }


    AffineTag compose(AffineTag newer, AffineTag older) {
        ++cnt.tag_compositions;
        if (exact())
            return {checked(Wide(newer.a) * older.a), checked(Wide(newer.a) * older.b + newer.b)};
        return {norm(Wide(newer.a) * older.a), norm(Wide(newer.a) * older.b + newer.b)};
    }

    Value apply_value(AffineTag t, Value x) const {
        if (exact()) return checked(Wide(t.a) * x + t.b);
        return norm(Wide(t.a) * x + t.b);
    }


    static void map_extrema(Value a, Value b, Value mn, Value mx, Value& omn, Value& omx) {
        if (a == 1) {
            omn = checked(Wide(mn) + b);
            omx = checked(Wide(mx) + b);
        } else if (a == -1) {
            omn = checked(-Wide(mx) + b);
            omx = checked(-Wide(mn) + b);
        } else {
            omn = omx = b;
        }
    }


    void map_aggregate(AffineTag t, Wide raw_sum, std::size_t len, Value rmn, Value rmx,
                       Residue& sum, Value& mn, Value& mx) const {
        sum = norm(Wide(t.a) * raw_sum + Wide(t.b) * Wide(len));
        if (exact()) map_extrema(t.a, t.b, rmn, rmx, mn, mx);
    }


    void scan(std::size_t lo, std::size_t hi, Wide& s, Value& mn, Value& mx) const {
        s = 0;
        if (exact()) {
            mn = std::numeric_limits<Value>::max();
            mx = std::numeric_limits<Value>::min();
            for (std::size_t i = lo; i < hi; ++i) {
                Value x = raw[i];
                s += x;
                if (x < mn) mn = x;
                if (x > mx) mx = x;
            }
        } else {
            for (std::size_t i = lo; i < hi; ++i) s += raw[i];
        }
    }

    void build(const std::vector<Value>& initial, Mode mode) {
        built = false;
        std::size_t nn = initial.size();
        std::size_t bs = g_block_setting;
        if (bs == 0) {
            bs = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<double>(nn))));
            while (bs * bs < nn) ++bs;
        }
        if (bs < 1) bs = 1;
        std::vector<Value> r;
        r.reserve(nn);
        for (Value v : initial) r.push_back(mode == Mode::exact_extrema ? v : norm(v));
        std::size_t nb = nn == 0 ? 0 : (nn + bs - 1) / bs;
        std::vector<Block> bl(nb);

        raw = std::move(r);
        blocks = std::move(bl);
        n = nn; bsize = bs; nblocks = nb; md = mode;
        for (std::size_t k = 0; k < nblocks; ++k) {
            Wide s; Value mn = 0, mx = 0;
            scan(bstart(k), bend(k), s, mn, mx);
            blocks[k].sum = norm(s);
            if (exact()) { blocks[k].mn = mn; blocks[k].mx = mx; }
        }
        cnt = {};
        built = true;
    }

    void require_built() const {
        if (!built) throw std::logic_error("SqrtDecomposition: build() has not been called");
    }
    void check_range(Range r, const char* what) const {
        if (r.begin > r.end || r.end > n) throw std::out_of_range(what);
    }


    void full_update(std::size_t k, AffineTag t) {
        ++cnt.full_blocks;
        Block& B = blocks[k];
        std::size_t len = bend(k) - bstart(k);
        Residue new_sum = norm(Wide(norm(t.a)) * B.sum + Wide(norm(t.b)) * Wide(len));
        Value nmn = B.mn, nmx = B.mx;
        if (exact()) map_extrema(t.a, t.b, B.mn, B.mx, nmn, nmx);
        AffineTag nt = compose(t, {B.a, B.b});
        B.sum = new_sum; B.mn = nmn; B.mx = nmx; B.a = nt.a; B.b = nt.b;
    }


    void partial_update(std::size_t k, std::size_t lo, std::size_t hi, AffineTag t) {
        Block& B = blocks[k];
        std::size_t bs = bstart(k), be = bend(k);
        AffineTag pend{B.a, B.b};
        bool has_pending = !is_identity(pend);
        Wide s = 0;
        Value mn = std::numeric_limits<Value>::max(), mx = std::numeric_limits<Value>::min();
        for (std::size_t i = bs; i < be; ++i) {
            Value x = raw[i];
            if (has_pending) x = apply_value(pend, x);
            if (i >= lo && i < hi) x = apply_value(t, x);
            raw[i] = x;
            s += x;
            if (x < mn) mn = x;
            if (x > mx) mx = x;
        }
        cnt.partial_elements += be - bs;
        B.a = 1; B.b = 0;
        B.sum = norm(s);
        if (exact()) { B.mn = mn; B.mx = mx; }
    }


    Aggregate partial_query(std::size_t k, std::size_t lo, std::size_t hi) {
        const Block& B = blocks[k];
        Wide s; Value rmn = 0, rmx = 0;
        scan(lo, hi, s, rmn, rmx);
        cnt.partial_elements += hi - lo;
        Aggregate out;
        out.length = hi - lo;
        Value mn = 0, mx = 0;
        map_aggregate({B.a, B.b}, s, hi - lo, rmn, rmx, out.sum_mod, mn, mx);
        if (exact()) { out.minimum = mn; out.maximum = mx; }
        return out;
    }

    void update(Range r, AffineTag t) {
        require_built();
        check_range(r, "SqrtDecomposition::update: invalid range");
        if (exact() && !exact_gain_ok(t.a))
            throw std::invalid_argument("SqrtDecomposition::update: exact_extrema requires gain in {-1,0,1}");
        if (r.begin == r.end) return;
        if (!exact()) { t.a = norm(t.a); t.b = norm(t.b); }
        std::size_t kl = r.begin / bsize, kr = (r.end - 1) / bsize;
        for (std::size_t k = kl; k <= kr; ++k) {
            std::size_t lo = std::max(r.begin, bstart(k)), hi = std::min(r.end, bend(k));
            if (lo == bstart(k) && hi == bend(k)) full_update(k, t);
            else partial_update(k, lo, hi, t);
        }
    }

    Aggregate query(Range r) {
        require_built();
        check_range(r, "SqrtDecomposition::query: invalid range");
        if (r.begin == r.end) return {};
        Aggregate acc;
        bool first = true;
        auto fold = [&](const Aggregate& part) {
            if (first) { acc = part; first = false; return; }
            ++cnt.aggregate_merges;
            acc.length += part.length;
            acc.sum_mod = norm(Wide(acc.sum_mod) + part.sum_mod);
            if (exact()) {
                acc.minimum = std::min(*acc.minimum, *part.minimum);
                acc.maximum = std::max(*acc.maximum, *part.maximum);
            }
        };
        std::size_t kl = r.begin / bsize, kr = (r.end - 1) / bsize;
        for (std::size_t k = kl; k <= kr; ++k) {
            std::size_t lo = std::max(r.begin, bstart(k)), hi = std::min(r.end, bend(k));
            if (lo == bstart(k) && hi == bend(k)) {
                ++cnt.full_blocks;
                const Block& B = blocks[k];
                Aggregate part;
                part.length = hi - lo;
                part.sum_mod = B.sum;
                if (exact()) { part.minimum = B.mn; part.maximum = B.mx; }
                fold(part);
            } else {
                fold(partial_query(k, lo, hi));
            }
        }
        return acc;
    }

    InvariantReport check() const {
        InvariantReport rep{true, {}};
        auto bad = [&](std::string m) { rep.ok = false; rep.violations.push_back(std::move(m)); };
        if (!built) { bad("not built"); return rep; }
        if (raw.size() != n) bad("raw size != n");
        if (blocks.size() != (n == 0 ? 0 : (n + bsize - 1) / bsize)) { bad("block count mismatch"); return rep; }
        for (std::size_t k = 0; k < nblocks; ++k) {
            const Block& B = blocks[k];
            std::string tag = " in block " + std::to_string(k);
            if (B.sum < 0 || B.sum >= modulus) bad("sum not canonical" + tag);
            if (exact()) {
                if (!exact_gain_ok(B.a)) bad("pending gain outside {-1,0,1}" + tag);
            } else {
                if (B.a < 0 || B.a >= modulus || B.b < 0 || B.b >= modulus) bad("pending tag not canonical" + tag);
            }
            Wide s = 0;
            Value mn = std::numeric_limits<Value>::max(), mx = std::numeric_limits<Value>::min();
            bool overflow = false;
            for (std::size_t i = bstart(k); i < bend(k); ++i) {
                Value x = raw[i];
                if (!exact() && (x < 0 || x >= modulus)) bad("raw residue not canonical" + tag);
                Wide y = Wide(B.a) * x + B.b;
                if (exact()) {
                    if (y < std::numeric_limits<Value>::min() || y > std::numeric_limits<Value>::max()) { overflow = true; break; }
                    mn = std::min(mn, static_cast<Value>(y));
                    mx = std::max(mx, static_cast<Value>(y));
                }
                s += y;
            }
            if (overflow) { bad("logical value overflows int64" + tag); continue; }
            if (norm(s) != B.sum) bad("block sum != recomputed logical sum" + tag);
            if (exact() && (B.mn != mn || B.mx != mx)) bad("block extrema != recomputed logical extrema" + tag);
        }
        return rep;
=======
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
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)
    }
};

SqrtDecomposition::SqrtDecomposition()
    :impl_(std::make_unique<Impl>()){}

<<<<<<< HEAD
void SqrtDecomposition::build(const std::vector<Value>& initial, Mode mode) { impl_->build(initial, mode); }
std::size_t SqrtDecomposition::size() const { impl_->require_built(); return impl_->n; }
Mode SqrtDecomposition::mode() const { impl_->require_built(); return impl_->md; }
void SqrtDecomposition::update(Range range, AffineTag tag) { impl_->update(range, tag); }
Aggregate SqrtDecomposition::query(Range range) { return impl_->query(range); }
Counters SqrtDecomposition::counters() const noexcept { return impl_->cnt; }
void SqrtDecomposition::reset_counters() noexcept { impl_->cnt = {}; }
InvariantReport SqrtDecomposition::check_invariants() const { return impl_->check(); }

}
=======
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
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)
