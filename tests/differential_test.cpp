
#include "cs253/array_oracle.hpp"
#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/pipeline.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

using namespace cs253;

static void require(bool cond,const std::string& msg){
    if(!cond)throw std::runtime_error(msg);
}

// ================================================================
// Factories
// ================================================================

static Factory make_a(){
    return []()->std::unique_ptr<RangeStructure>{
        return std::make_unique<LazySegmentTree>();
    };
}

static Factory make_b(){
    return []()->std::unique_ptr<RangeStructure>{
        return std::make_unique<SqrtDecomposition>();
    };
}

// ================================================================
// Deliberately faulty structure
//
// It delegates all operations to a correct SqrtDecomposition,
// but adds 1 to the sum of every non-empty query.
//
// This simulates a real implementation bug without modifying
// Structure A or Structure B.
// ================================================================

class FaultyStructure:public RangeStructure{
private:
    SqrtDecomposition impl;

public:
    void build(const std::vector<Value>& initial,Mode mode) override{
        impl.build(initial,mode);
    }

    std::size_t size() const override{
        return impl.size();
    }

    Mode mode() const override{
        return impl.mode();
    }

    void update(Range range,AffineTag tag) override{
        impl.update(range,tag);
    }

    Aggregate query(Range range) override{
        Aggregate result=impl.query(range);

        if(result.length>0){
            result.sum_mod=(result.sum_mod+1)%modulus;
        }

        return result;
    }

    Counters counters() const noexcept override{
        return impl.counters();
    }

    void reset_counters() noexcept override{
        impl.reset_counters();
    }

    InvariantReport check_invariants() const override{
        return impl.check_invariants();
    }
};

static Factory make_faulty(){
    return []()->std::unique_ptr<RangeStructure>{
        return std::make_unique<FaultyStructure>();
    };
}

// ================================================================
// Aggregate comparison
// ================================================================

static bool aggregates_equal(const Aggregate& x,const Aggregate& y){
    return x.length==y.length
        && x.sum_mod==y.sum_mod
        && x.minimum==y.minimum
        && x.maximum==y.maximum;
}

// ================================================================
// Replay a workload and check whether the faulty structure
// still disagrees with the Oracle.
//
// This is used to independently verify the shrunk workload.
// ================================================================

static bool workload_reproduces_failure(const Workload& wl){
    ArrayOracle oracle;
    FaultyStructure faulty;

    oracle.build(wl.initial,wl.mode);
    faulty.build(wl.initial,wl.mode);

    for(const Operation& operation:wl.operations){
        if(const auto* update=std::get_if<Update>(&operation)){
            try{
                oracle.update(update->range,update->tag);
                faulty.update(update->range,update->tag);
            }
            catch(...){
                return true;
            }
        }
        else{
            const auto& query=std::get<Query>(operation);

            try{
                Aggregate expected=oracle.query(query.range);
                Aggregate actual=faulty.query(query.range);

                if(!aggregates_equal(expected,actual)){
                    return true;
                }
            }
            catch(...){
                return true;
            }
        }

        if(!oracle.check_invariants().ok ||
           !faulty.check_invariants().ok){
            return true;
        }
    }

    return false;
}

// ================================================================
// Exact mode: 1,000,000 operations
// ================================================================

static void test_exact_mode(){
    DifferentialConfig cfg;
    cfg.seed=253;
    cfg.elements=128;
    cfg.operations=1000000;
    cfg.mode=Mode::exact_extrema;

    auto result=differential_test(make_a(),make_b(),cfg);

    require(
        result.passed,
        "exact_extrema FAILED at op "+
        std::to_string(result.operations_checked)+": "+
        result.message
    );

    require(
        result.operations_checked==cfg.operations,
        "Wrong exact-mode operation count"
    );

    std::cout<<"PASS  exact_extrema   "
             <<result.operations_checked<<" ops"<<std::endl;
}

// ================================================================
// Modular mode: 1,000,000 operations
// ================================================================

static void test_modular_mode(){
    DifferentialConfig cfg;
    cfg.seed=42;
    cfg.elements=128;
    cfg.operations=1000000;
    cfg.mode=Mode::modular_sum;

    auto result=differential_test(make_a(),make_b(),cfg);

    require(
        result.passed,
        "modular_sum FAILED at op "+
        std::to_string(result.operations_checked)+": "+
        result.message
    );

    require(
        result.operations_checked==cfg.operations,
        "Wrong modular-mode operation count"
    );

    std::cout<<"PASS  modular_sum     "
             <<result.operations_checked<<" ops"<<std::endl;
}

// ================================================================
// Edge cases: n=0,1,2,3
// ================================================================

static void test_edge_cases(){
    struct TC{
        std::size_t n;
        std::size_t ops;
        Mode mode;
        Seed seed;
    };

    const TC cases[]={
        {0,200,Mode::exact_extrema,1},
        {0,200,Mode::modular_sum,2},
        {1,2000,Mode::exact_extrema,3},
        {1,2000,Mode::modular_sum,4},
        {2,2000,Mode::exact_extrema,5},
        {3,2000,Mode::modular_sum,6}
    };

    for(const auto& tc:cases){
        DifferentialConfig cfg;
        cfg.seed=tc.seed;
        cfg.elements=tc.n;
        cfg.operations=tc.ops;
        cfg.mode=tc.mode;

        auto result=differential_test(make_a(),make_b(),cfg);

        require(
            result.passed,
            "Edge case n="+std::to_string(tc.n)+
            " failed at op "+
            std::to_string(result.operations_checked)+
            ": "+result.message
        );
    }

    std::cout<<"PASS  edge_cases (n in {0,1,2,3})"<<std::endl;
}

// ================================================================
// Determinism
// ================================================================

static void test_determinism(){
    DifferentialConfig cfg;
    cfg.seed=999;
    cfg.elements=32;
    cfg.operations=5000;
    cfg.mode=Mode::exact_extrema;

    auto r1=differential_test(make_a(),make_b(),cfg);
    auto r2=differential_test(make_a(),make_b(),cfg);

    require(r1.passed==r2.passed,"Determinism broken");
    require(
        r1.operations_checked==r2.operations_checked,
        "Determinism: operation counts differ"
    );

    require(r1.message==r2.message,"Determinism: messages differ");

    std::cout<<"PASS  determinism"<<std::endl;
}

// ================================================================
// Real automatic shrinking test
//
// The faulty structure returns an incorrect sum.
// differential_test must detect the mismatch and automatically
// shrink the failing workload.
// ================================================================

static void test_automatic_shrinking(){
    DifferentialConfig cfg;
    cfg.seed=253;
    cfg.elements=8;
    cfg.operations=500;
    cfg.mode=Mode::exact_extrema;

    auto result=differential_test(
        make_faulty(),
        make_b(),
        cfg
    );

    require(
        !result.passed,
        "Differential harness failed to detect faulty structure"
    );

    require(
        result.failing_case.has_value(),
        "Failure did not produce a failing workload"
    );

    const Workload& shrunk=*result.failing_case;

    require(
        !shrunk.operations.empty(),
        "Shrunk workload should contain a reproducing operation"
    );

    require(
        workload_reproduces_failure(shrunk),
        "Shrunk workload does not reproduce the original failure"
    );

    require(
        shrunk.operations.size()<=result.operations_checked,
        "Shrunk workload is larger than the original failure prefix"
    );

    std::cout<<"PASS  automatic_shrinking"<<std::endl;
    std::cout<<"      Original failure prefix: "
             <<result.operations_checked<<" operations"<<std::endl;
    std::cout<<"      Reduced workload: "
             <<shrunk.operations.size()<<" operations"<<std::endl;
    std::cout<<"      Failure: "<<result.message<<std::endl;
}

// ================================================================
// Standalone shrinker deletion-minimality test
// ================================================================

static void test_shrink_failure(){
    Workload wl;
    wl.mode=Mode::exact_extrema;
    wl.seed=7;
    wl.initial={10,20,30,40,50};

    wl.operations={
        Update{{0,5},{1,3}},
        Update{{0,3},{-1,7}},
        Query{{0,5}},
        Update{{1,4},{0,-2}},
        Query{{1,3}},
        Update{{0,5},{1,5}},
        Update{{2,5},{-1,0}},
        Query{{0,2}}
    };

    auto still_fails=[](const Workload& w)->bool{
        for(const auto& op:w.operations){
            if(const auto* u=std::get_if<Update>(&op)){
                if(u->tag.a==-1)return true;
            }
        }
        return false;
    };

    require(still_fails(wl),"Initial workload must fail");

    Workload shrunk=shrink_failure(wl,still_fails);

    require(
        still_fails(shrunk),
        "Shrunk workload must still fail"
    );

    require(
        shrunk.operations.size()<=wl.operations.size(),
        "Shrunk workload must not grow"
    );

    for(std::size_t i=0;i<shrunk.operations.size();i++){
        Workload without=shrunk;

        without.operations.erase(
            without.operations.begin()+
            static_cast<std::ptrdiff_t>(i)
        );

        require(
            !still_fails(without),
            "Shrunk result is not deletion-minimal"
        );
    }

    std::cout<<"PASS  shrink_failure ("
             <<wl.operations.size()<<" -> "
             <<shrunk.operations.size()
             <<" operations)"<<std::endl;
}

// ================================================================
// Main
// ================================================================

int main(){
    try{
        std::cout<<"=== Differential Test Harness (D4) ==="
                 <<std::endl;

        test_exact_mode();
        test_modular_mode();
        test_edge_cases();
        test_determinism();
        test_automatic_shrinking();
        test_shrink_failure();

        std::cout<<"All differential tests passed."
                 <<std::endl;

        return 0;
    }
    catch(const std::exception& e){
        std::cerr<<"FAIL: "<<e.what()<<std::endl;
        return 1;
    }
}
