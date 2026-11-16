
#include <string>
#include "cs253/array_oracle.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/pipeline.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace cs253 {

namespace {

using Wide=__int128_t;

Residue norm(Wide x){
    x%=modulus;
    if(x<0)x+=modulus;
    return static_cast<Residue>(x);
}

Value checked(Wide x){
    if(x<std::numeric_limits<Value>::min() ||
       x>std::numeric_limits<Value>::max()){
        throw std::overflow_error("ArrayOracle: integer overflow");
    }
    return static_cast<Value>(x);
}

}

struct ArrayOracle::Impl{
    std::vector<Value> arr;
    Mode current_mode=Mode::exact_extrema;
    bool built=false;
    Counters stats{};
};

ArrayOracle::ArrayOracle()
    :impl_(std::make_unique<Impl>()){}

ArrayOracle::~ArrayOracle()=default;

void ArrayOracle::build(
    const std::vector<Value>& initial,
    Mode requested_mode
){
    std::vector<Value> temp;
    temp.reserve(initial.size());

    for(Value x:initial){
        if(requested_mode==Mode::modular_sum)
            temp.push_back(norm(x));
        else
            temp.push_back(x);
    }

    impl_->arr=std::move(temp);
    impl_->current_mode=requested_mode;
    impl_->stats={};
    impl_->built=true;
}

std::size_t ArrayOracle::size() const{
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");
    return impl_->arr.size();
}

Mode ArrayOracle::mode() const{
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");
    return impl_->current_mode;
}

void ArrayOracle::update(Range range,AffineTag tag){
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");

    std::size_t n=impl_->arr.size();

    if(range.begin>range.end || range.end>n)
        throw std::out_of_range("ArrayOracle: invalid range");

    bool exact=(impl_->current_mode==Mode::exact_extrema);

    if(exact && tag.a!=-1 && tag.a!=0 && tag.a!=1)
        throw std::invalid_argument("ArrayOracle: invalid exact gain");

    // Validate all exact results before modifying anything.
    if(exact){
        for(std::size_t i=range.begin;i<range.end;i++)
            checked(Wide(tag.a)*impl_->arr[i]+tag.b);
    }

    for(std::size_t i=range.begin;i<range.end;i++){
        if(exact){
            impl_->arr[i]=checked(
                Wide(tag.a)*impl_->arr[i]+tag.b
            );
        }
        else{
            impl_->arr[i]=norm(
                Wide(norm(tag.a))*impl_->arr[i]+norm(tag.b)
            );
        }
    }
}

Aggregate ArrayOracle::query(Range range){
    if(!impl_->built)
        throw std::logic_error("ArrayOracle: build() not called");

    std::size_t n=impl_->arr.size();

    if(range.begin>range.end || range.end>n)
        throw std::out_of_range("ArrayOracle: invalid range");

    Aggregate result;
    result.length=range.end-range.begin;

    if(result.length==0)
        return result;

    Wide sum=0;
    Value mn=impl_->arr[range.begin];
    Value mx=impl_->arr[range.begin];

    for(std::size_t i=range.begin;i<range.end;i++){
        Value x=impl_->arr[i];
        sum+=x;
        if(x<mn)mn=x;
        if(x>mx)mx=x;
    }

    result.sum_mod=norm(sum);

    if(impl_->current_mode==Mode::exact_extrema){
        result.minimum=mn;
        result.maximum=mx;
    }

    return result;
}

Counters ArrayOracle::counters() const noexcept{
    return impl_->stats;
}

void ArrayOracle::reset_counters() noexcept{
    impl_->stats={};
}

InvariantReport ArrayOracle::check_invariants() const{
    return InvariantAccess::check(*this);
}

InvariantReport InvariantAccess::check(const ArrayOracle& oracle){
    InvariantReport report{true,{}};
    const auto& p=*oracle.impl_;

    auto fail=[&](const std::string& message){
        report.ok=false;
        report.violations.push_back(message);
    };

    if(!p.built){
        fail("ArrayOracle has not been built");
        return report;
    }

    for(std::size_t i=0;i<p.arr.size();i++){
        Value x=p.arr[i];

        if(p.current_mode==Mode::modular_sum){
            if(x<0 || x>=modulus){
                fail("Non-canonical modular value at index "+
                     std::to_string(i));
            }
        }
    }

    return report;
}

// ================================================================
// Structure A invariant checker
// ================================================================

InvariantReport InvariantAccess::check(const LazySegmentTree& t){
    InvariantReport report{true,{}};

    auto fail=[&](std::string msg){
        report.ok=false;
        report.violations.push_back(std::move(msg));
    };

    if(t.tree.size()!=4*t.n+4){
        fail("tree storage size mismatch");
        return report;
    }

    if(t.n==0)return report;

    auto chk=[&](auto& self,int v,int tl,int tr)->void{
        const auto& nd=t.tree[static_cast<std::size_t>(v)];

        if(nd.sum<0 || nd.sum>=modulus)
            fail("sum not in [0,P) at node "+std::to_string(v));

        if(t.current_mode==Mode::exact_extrema){
            if(nd.lazy.a!=-1 && nd.lazy.a!=0 && nd.lazy.a!=1)
                fail("invalid exact lazy gain at node "+std::to_string(v));

            if(nd.mn>nd.mx)
                fail("min > max at node "+std::to_string(v));
        }
        else{
            if(nd.lazy.a<0 || nd.lazy.a>=modulus)
                fail("lazy.a not canonical at node "+std::to_string(v));

            if(nd.lazy.b<0 || nd.lazy.b>=modulus)
                fail("lazy.b not canonical at node "+std::to_string(v));
        }

        if(tl==tr){
            if(t.current_mode==Mode::exact_extrema &&
               (nd.mn!=nd.mx || nd.sum!=norm(nd.mn))){
                fail("leaf aggregate inconsistency at node "+
                     std::to_string(v));
            }
            return;
        }

        int tm=tl+(tr-tl)/2;

        self(self,2*v,tl,tm);
        self(self,2*v+1,tm+1,tr);

        const auto& L=t.tree[static_cast<std::size_t>(2*v)];
        const auto& R=t.tree[static_cast<std::size_t>(2*v+1)];

        int len=tr-tl+1;

        Residue child_sum=norm(Wide(L.sum)+R.sum);

        Residue exp_sum=norm(
            Wide(norm(nd.lazy.a))*child_sum+
            Wide(norm(nd.lazy.b))*Wide(len)
        );

        if(nd.sum!=exp_sum)
            fail("parent sum/lazy inconsistency at node "+
                 std::to_string(v));

        if(t.current_mode==Mode::exact_extrema){
            Value cmin=std::min(L.mn,R.mn);
            Value cmax=std::max(L.mx,R.mx);

            Wide exp_min,exp_max;

            if(nd.lazy.a==1){
                exp_min=Wide(cmin)+nd.lazy.b;
                exp_max=Wide(cmax)+nd.lazy.b;
            }
            else if(nd.lazy.a==-1){
                exp_min=-Wide(cmax)+nd.lazy.b;
                exp_max=-Wide(cmin)+nd.lazy.b;
            }
            else{
                exp_min=Wide(nd.lazy.b);
                exp_max=Wide(nd.lazy.b);
            }

            if(Wide(nd.mn)!=exp_min || Wide(nd.mx)!=exp_max)
                fail("parent extrema/lazy inconsistency at node "+
                     std::to_string(v));
        }
    };

    chk(chk,1,0,static_cast<int>(t.n)-1);
    return report;
}

// ================================================================
// Structure B invariant checker
// ================================================================

InvariantReport InvariantAccess::check(const SqrtDecomposition& s){
    return s.check_invariants();
}

namespace {

bool aggregates_equal(const Aggregate& x,const Aggregate& y){
    return x.length==y.length
        && x.sum_mod==y.sum_mod
        && x.minimum==y.minimum
        && x.maximum==y.maximum;
}

// Replay a workload using fresh structures.
// Returns true if the candidate still reproduces a failure.

bool reproduces_failure(
    const Factory& factory_a,
    const Factory& factory_b,
    const Workload& workload
){
    try{
        ArrayOracle oracle;
        auto a=factory_a();
        auto b=factory_b();

        if(!a || !b)return true;

        oracle.build(workload.initial,workload.mode);
        a->build(workload.initial,workload.mode);
        b->build(workload.initial,workload.mode);

        auto invariants_ok=[&](){
            return oracle.check_invariants().ok
                && a->check_invariants().ok
                && b->check_invariants().ok;
        };

        if(!invariants_ok())return true;

        for(const Operation& operation:workload.operations){
            if(const auto* update=std::get_if<Update>(&operation)){
                oracle.update(update->range,update->tag);
                a->update(update->range,update->tag);
                b->update(update->range,update->tag);
            }
            else{
                const auto& query=std::get<Query>(operation);

                Aggregate expected=oracle.query(query.range);
                Aggregate actual_a=a->query(query.range);
                Aggregate actual_b=b->query(query.range);

                if(!aggregates_equal(expected,actual_a) ||
                   !aggregates_equal(expected,actual_b)){
                    return true;
                }
            }

            if(!invariants_ok())return true;
        }
    }
    catch(...){
        return true;
    }

    return false;
}

}

// ================================================================
// Differential testing
// ================================================================

DifferentialResult differential_test(
    const Factory& factory_a,
    const Factory& factory_b,
    const DifferentialConfig& config
){
    DifferentialResult result;
    result.passed=false;
    result.operations_checked=0;

    auto oracle=std::make_unique<ArrayOracle>();
    auto struct_a=factory_a();
    auto struct_b=factory_b();

    if(!struct_a || !struct_b){
        result.message="A factory returned a null structure";
        return result;
    }

    std::mt19937_64 rng(config.seed);
    const std::size_t n=config.elements;

    std::vector<Value> initial;
    initial.reserve(n);

    for(std::size_t i=0;i<n;i++){
        initial.push_back(
            static_cast<Value>(
                static_cast<int64_t>(rng()%201u)-100
            )
        );
    }

    oracle->build(initial,config.mode);
    struct_a->build(initial,config.mode);
    struct_b->build(initial,config.mode);

    Workload wl;
    wl.mode=config.mode;
    wl.seed=config.seed;
    wl.initial=initial;
    wl.operations.reserve(config.operations);

    auto make_fail=[&](std::string msg,std::size_t idx){
        DifferentialResult r;
        r.passed=false;
        r.operations_checked=idx;
        r.message=std::move(msg);

        Workload failing_prefix=wl;

        auto still_fails=[&](const Workload& candidate){
            return reproduces_failure(
                factory_a,factory_b,candidate
            );
        };

        if(still_fails(failing_prefix)){
            r.failing_case=shrink_failure(
                failing_prefix,still_fails
            );
        }
        else{
            r.failing_case=std::move(failing_prefix);
        }

        return r;
    };

    for(std::size_t i=0;i<config.operations;i++){
        bool do_update=(n>0) && ((rng()&1u)==0u);

        if(do_update){
            std::size_t l=static_cast<std::size_t>(rng()%(n+1u));
            std::size_t len=static_cast<std::size_t>(
                rng()%(n-l+1u)
            );

            Range range{l,l+len};
            AffineTag tag;

            if(config.mode==Mode::exact_extrema){
                const Value gains[3]={-1,0,1};
                tag.a=gains[rng()%3u];
                tag.b=static_cast<Value>(
                    static_cast<int64_t>(rng()%101u)-50
                );
            }
            else{
                tag.a=static_cast<Value>(rng());
                tag.b=static_cast<Value>(rng());
            }

            try{
                oracle->update(range,tag);
            }
            catch(const std::exception& e){
                return make_fail(
                    std::string("Oracle::update threw: ")+e.what(),i
                );
            }

            bool a_threw=false,b_threw=false;
            std::string a_msg,b_msg;

            try{
                struct_a->update(range,tag);
            }
            catch(const std::exception& e){
                a_threw=true;
                a_msg=e.what();
            }

            try{
                struct_b->update(range,tag);
            }
            catch(const std::exception& e){
                b_threw=true;
                b_msg=e.what();
            }

            wl.operations.push_back(Update{range,tag});

            if(a_threw){
                return make_fail(
                    "A::update threw at op "+std::to_string(i)+
                    ": "+a_msg,i+1
                );
            }

            if(b_threw){
                return make_fail(
                    "B::update threw at op "+std::to_string(i)+
                    ": "+b_msg,i+1
                );
            }

            auto ra=struct_a->check_invariants();

            if(!ra.ok){
                return make_fail(
                    "A invariant violated after update at op "+
                    std::to_string(i)+": "+
                    (ra.violations.empty()?"(no detail)":ra.violations[0]),
                    i+1
                );
            }

            auto rb=struct_b->check_invariants();

            if(!rb.ok){
                return make_fail(
                    "B invariant violated after update at op "+
                    std::to_string(i)+": "+
                    (rb.violations.empty()?"(no detail)":rb.violations[0]),
                    i+1
                );
            }
        }
        else{
            std::size_t l=(n==0)?0u:
                static_cast<std::size_t>(rng()%(n+1u));

            std::size_t len=(n==0)?0u:
                static_cast<std::size_t>(rng()%(n-l+1u));

            Range range{l,l+len};

            Aggregate o_agg,a_agg,b_agg;

            try{
                o_agg=oracle->query(range);
            }
            catch(const std::exception& e){
                return make_fail(
                    std::string("Oracle::query threw: ")+e.what(),i
                );
            }

            bool a_threw=false,b_threw=false;
            std::string a_msg,b_msg;

            try{
                a_agg=struct_a->query(range);
            }
            catch(const std::exception& e){
                a_threw=true;
                a_msg=e.what();
            }

            try{
                b_agg=struct_b->query(range);
            }
            catch(const std::exception& e){
                b_threw=true;
                b_msg=e.what();
            }

            wl.operations.push_back(Query{range});

            if(a_threw){
                return make_fail(
                    "A::query threw at op "+std::to_string(i)+
                    ": "+a_msg,i+1
                );
            }

            if(b_threw){
                return make_fail(
                    "B::query threw at op "+std::to_string(i)+
                    ": "+b_msg,i+1
                );
            }

            if(!aggregates_equal(o_agg,a_agg)){
                return make_fail(
                    "A disagrees with Oracle on query at op "+
                    std::to_string(i),i+1
                );
            }

            if(!aggregates_equal(o_agg,b_agg)){
                return make_fail(
                    "B disagrees with Oracle on query at op "+
                    std::to_string(i),i+1
                );
            }

            auto ra=struct_a->check_invariants();

            if(!ra.ok){
                return make_fail(
                    "A invariant violated after query at op "+
                    std::to_string(i)+": "+
                    (ra.violations.empty()?"(no detail)":ra.violations[0]),
                    i+1
                );
            }

            auto rb=struct_b->check_invariants();

            if(!rb.ok){
                return make_fail(
                    "B invariant violated after query at op "+
                    std::to_string(i)+": "+
                    (rb.violations.empty()?"(no detail)":rb.violations[0]),
                    i+1
                );
            }
        }

        result.operations_checked=i+1;
    }

    result.passed=true;
    result.message="All "+
        std::to_string(config.operations)+" operations passed.";

    return result;
}

// ================================================================
// Failure shrinking
// ================================================================

Workload shrink_failure(
    const Workload& failure,
    const std::function<bool(const Workload&)>& still_fails
){
    Workload current=failure;

    if(!still_fails(current))
        return current;

    // Phase 1: chunk-based delta debugging.
    std::size_t granularity=2;

    while(!current.operations.empty()){
        std::size_t count=current.operations.size();
        granularity=std::min(granularity,count);

        std::size_t chunk=(count+granularity-1)/granularity;
        bool reduced=false;

        for(std::size_t begin=0;begin<count;begin+=chunk){
            std::size_t end=std::min(count,begin+chunk);

            Workload candidate=current;

            candidate.operations.erase(
                candidate.operations.begin()+
                    static_cast<std::ptrdiff_t>(begin),
                candidate.operations.begin()+
                    static_cast<std::ptrdiff_t>(end)
            );

            if(still_fails(candidate)){
                current=std::move(candidate);
                granularity=std::max<std::size_t>(
                    2,granularity-1
                );
                reduced=true;
                break;
            }
        }

        if(reduced)continue;

        if(granularity==count)break;

        granularity=std::min(count,granularity*2);
    }

    // Phase 2: simplify initial values toward zero.
    for(std::size_t i=0;i<current.initial.size();i++){
        if(current.initial[i]==0)continue;

        Workload candidate=current;
        candidate.initial[i]=0;

        if(still_fails(candidate))
            current=std::move(candidate);
    }

    // Phase 3: simplify operations.
    for(std::size_t i=0;i<current.operations.size();i++){
        if(!std::holds_alternative<Update>(current.operations[i])){
            const auto& op=std::get<Query>(current.operations[i]);

            if(op.range.end>op.range.begin+1){
                Workload candidate=current;

                --std::get<Query>(
                    candidate.operations[i]
                ).range.end;

                if(still_fails(candidate))
                    current=std::move(candidate);
            }

            continue;
        }

        // Try b=0.
        if(std::get<Update>(current.operations[i]).tag.b!=0){
            Workload candidate=current;

            std::get<Update>(
                candidate.operations[i]
            ).tag.b=0;

            if(still_fails(candidate))
                current=std::move(candidate);
        }

        // Try a=1.
        if(std::get<Update>(current.operations[i]).tag.a!=1){
            Workload candidate=current;

            std::get<Update>(
                candidate.operations[i]
            ).tag.a=1;

            if(still_fails(candidate))
                current=std::move(candidate);
        }

        // Narrow range end.
        {
            const auto& op=std::get<Update>(
                current.operations[i]
            );

            if(op.range.end>op.range.begin+1){
                Workload candidate=current;

                --std::get<Update>(
                    candidate.operations[i]
                ).range.end;

                if(still_fails(candidate))
                    current=std::move(candidate);
            }
        }

        // Narrow range beginning.
        {
            const auto& op=std::get<Update>(
                current.operations[i]
            );

            if(op.range.begin+1<op.range.end){
                Workload candidate=current;

                ++std::get<Update>(
                    candidate.operations[i]
                ).range.begin;

                if(still_fails(candidate))
                    current=std::move(candidate);
            }
        }
    }

    return current;
}

} // namespace cs253
