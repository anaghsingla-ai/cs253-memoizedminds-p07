
#include "cs253/array_oracle.hpp"
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cs253;

void require(bool condition,const std::string& message){
    if(!condition)throw std::runtime_error(message);
}

template<typename Exception,typename Function>
void expect_throw(Function fn,const std::string& message){
    bool thrown=false;
    try{
        fn();
    }
    catch(const Exception&){
        thrown=true;
    }
    require(thrown,message);
}

void test_build_and_basic_query(){
    ArrayOracle oracle;
    oracle.build({1,2,3,4,5},Mode::exact_extrema);

    require(oracle.size()==5,"Incorrect size");
    require(oracle.mode()==Mode::exact_extrema,"Incorrect mode");

    Aggregate result=oracle.query({1,4});

    require(result.length==3,"Incorrect query length");
    require(result.sum_mod==9,"Incorrect sum");
    require(result.minimum==2,"Incorrect minimum");
    require(result.maximum==4,"Incorrect maximum");
}

void test_exact_affine_updates(){
    ArrayOracle oracle;
    oracle.build({1,2,3,4,5},Mode::exact_extrema);

    oracle.update({1,4},{-1,10});

    Aggregate result=oracle.query({0,5});

    require(result.sum_mod==27,"Incorrect sum after negative update");
    require(result.minimum==1,"Incorrect minimum after negative update");
    require(result.maximum==8,"Incorrect maximum after negative update");

    oracle.update({0,5},{0,-3});

    result=oracle.query({0,5});

    require(result.sum_mod==modulus-15,"Incorrect sum after zero multiplier");
    require(result.minimum==-3,"Incorrect minimum after zero multiplier");
    require(result.maximum==-3,"Incorrect maximum after zero multiplier");
}

void test_modular_mode(){
    ArrayOracle oracle;
    oracle.build({-1,2,modulus+3},Mode::modular_sum);

    Aggregate result=oracle.query({0,3});

    require(result.length==3,"Incorrect modular query length");
    require(result.sum_mod==4,"Incorrect initial modular sum");
    require(!result.minimum.has_value(),"Modular mode must not return minimum");
    require(!result.maximum.has_value(),"Modular mode must not return maximum");

    oracle.update({0,3},{-2,-3});

    result=oracle.query({0,3});

    // Values become:
    // -2*(-1)-3 = -1
    // -2*2-3 = -7
    // -2*3-3 = -9
    // Their canonical residues sum to modulus-17.
    require(result.sum_mod==modulus-17,"Incorrect modular affine update");
    require(oracle.check_invariants().ok,"Modular invariant check failed");
}

void test_empty_and_boundary_ranges(){
    ArrayOracle oracle;
    oracle.build({10,20,30},Mode::exact_extrema);

    Aggregate empty=oracle.query({1,1});

    require(empty.length==0,"Empty query has incorrect length");
    require(empty.sum_mod==0,"Empty query has nonzero sum");
    require(!empty.minimum.has_value(),"Empty query has a minimum");
    require(!empty.maximum.has_value(),"Empty query has a maximum");

    oracle.update({0,0},{1,100});
    oracle.update({3,3},{1,100});
    oracle.update({0,3},{1,1});

    Aggregate result=oracle.query({0,3});

    require(result.sum_mod==63,"Incorrect full-range update");
    require(result.minimum==11,"Incorrect minimum after boundary updates");
    require(result.maximum==31,"Incorrect maximum after boundary updates");
}

void test_invalid_ranges(){
    ArrayOracle oracle;
    oracle.build({1,2,3},Mode::exact_extrema);

    expect_throw<std::out_of_range>(
        [&](){oracle.update({2,4},{1,1});},
        "Invalid update range did not throw"
    );

    expect_throw<std::out_of_range>(
        [&](){oracle.update({2,1},{1,1});},
        "Reversed update range did not throw"
    );

    expect_throw<std::out_of_range>(
        [&](){oracle.query({0,4});},
        "Invalid query range did not throw"
    );

    Aggregate result=oracle.query({0,3});
    require(result.sum_mod==6,"Invalid update modified the array");
}

void test_invalid_exact_multiplier(){
    ArrayOracle oracle;
    oracle.build({1,2,3},Mode::exact_extrema);

    expect_throw<std::invalid_argument>(
        [&](){oracle.update({0,3},{2,5});},
        "Invalid exact multiplier did not throw"
    );

    Aggregate result=oracle.query({0,3});
    require(result.sum_mod==6,"Invalid multiplier modified the array");
}
void test_overflow_is_atomic(){
    ArrayOracle oracle;

    oracle.build({
        std::numeric_limits<Value>::max(),
        10
    },Mode::exact_extrema);

    expect_throw<std::overflow_error>(
        [&](){oracle.update({0,2},{1,1});},
        "Overflow did not throw"
    );

    Aggregate result=oracle.query({0,2});

    require(result.sum_mod==
            (std::numeric_limits<Value>::max()%modulus+10)%modulus,
            "Overflowing update partially modified the array");

    require(result.maximum==std::numeric_limits<Value>::max(),
            "Overflowing update changed the maximum");
}

void test_unbuilt_operations(){
    ArrayOracle oracle;

    expect_throw<std::logic_error>(
        [&](){oracle.update({0,0},{1,1});},
        "Update before build did not throw"
    );

    expect_throw<std::logic_error>(
        [&](){oracle.query({0,0});},
        "Query before build did not throw"
    );

    require(!oracle.check_invariants().ok,
            "Unbuilt oracle should fail invariant check");
}

void test_rebuild_and_counters(){
    ArrayOracle oracle;

    oracle.build({1,2,3},Mode::exact_extrema);
    oracle.update({0,3},{1,5});
    oracle.reset_counters();

    Counters counters=oracle.counters();

    require(counters.nodes_visited==0,"Oracle node counter must be zero");
    require(counters.push_downs==0,"Oracle push counter must be zero");
    require(counters.full_blocks==0,"Oracle block counter must be zero");
    require(counters.partial_elements==0,"Oracle partial counter must be zero");
    require(counters.tag_compositions==0,"Oracle composition counter must be zero");
    require(counters.aggregate_merges==0,"Oracle merge counter must be zero");

    oracle.build({7,8},Mode::exact_extrema);

    Aggregate result=oracle.query({0,2});

    require(result.sum_mod==15,"Build did not replace previous array");
    require(oracle.check_invariants().ok,"Invariant check failed after rebuild");
}

int main(){
    try{
        test_build_and_basic_query();
        test_exact_affine_updates();
        test_modular_mode();
        test_empty_and_boundary_ranges();
        test_invalid_ranges();
        test_invalid_exact_multiplier();
        test_unbuilt_operations();
        test_rebuild_and_counters();

        std::cout<<"All ArrayOracle tests passed!"<<std::endl;
    }
    catch(const std::exception& e){
        std::cerr<<"Oracle test failed: "<<e.what()<<std::endl;
        return 1;
    }

    return 0;
}
