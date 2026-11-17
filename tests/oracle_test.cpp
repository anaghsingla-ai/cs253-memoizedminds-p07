
#include "cs253/array_oracle.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cs253;

void require(bool condition,const std::string& message){
    if(!condition){
        throw std::runtime_error(message);
    }
}

template<typename Exception,typename Function>
void expect_throw(Function function,const std::string& message){
    bool thrown=false;
    try{
        function();
    }
    catch(const Exception&){
        thrown=true;
    }
    require(thrown,message);
}

void test_build_and_basic_query(){
    ArrayOracle oracle;
    oracle.build({1,2,3,4,5},Mode::exact_extrema);

    Aggregate result=oracle.query({0,5});

    require(result.length==5,"Incorrect query length");
    require(result.sum_mod==15,"Incorrect sum");
    require(result.minimum.has_value() && *result.minimum==1,"Incorrect minimum");
    require(result.maximum.has_value() && *result.maximum==5,"Incorrect maximum");
}

void test_exact_affine_updates(){
    ArrayOracle oracle;
    oracle.build({1,2,3,4,5},Mode::exact_extrema);

    oracle.update({1,4},{-1,10});

    Aggregate result=oracle.query({0,5});

    require(result.sum_mod==27,"Incorrect sum after negative affine update");
    require(result.minimum.has_value() && *result.minimum==1,"Incorrect minimum after update");
    require(result.maximum.has_value() && *result.maximum==8,"Incorrect maximum after update");

    oracle.update({0,5},{0,-3});

    result=oracle.query({0,5});

    require(result.sum_mod==modulus-15,"Incorrect sum after assignment update");
    require(result.minimum.has_value() && *result.minimum==-3,"Incorrect minimum after assignment");
    require(result.maximum.has_value() && *result.maximum==-3,"Incorrect maximum after assignment");
}

void test_modular_mode(){
    ArrayOracle oracle;
    oracle.build({modulus+1,-1},Mode::modular_sum);

    Aggregate result=oracle.query({0,2});

    require(result.length==2,"Incorrect modular query length");
    require(result.sum_mod==0,"Incorrect modular normalization during build");
    require(!result.minimum.has_value(),"Minimum should be absent in modular mode");
    require(!result.maximum.has_value(),"Maximum should be absent in modular mode");

    oracle.update({0,2},{2,3});

    result=oracle.query({0,2});

    require(result.sum_mod==6,"Incorrect modular affine update");
    require(!result.minimum.has_value(),"Minimum should remain absent in modular mode");
    require(!result.maximum.has_value(),"Maximum should remain absent in modular mode");
}

void test_empty_and_boundary_ranges(){
    ArrayOracle oracle;
    oracle.build({4,8,12},Mode::exact_extrema);

    Aggregate empty=oracle.query({1,1});

    require(empty.length==0,"Empty range should have length zero");
    require(empty.sum_mod==0,"Empty range should have sum zero");
    require(!empty.minimum.has_value(),"Empty range should have no minimum");
    require(!empty.maximum.has_value(),"Empty range should have no maximum");

    Aggregate first=oracle.query({0,1});
    require(first.sum_mod==4,"Incorrect first-element query");

    Aggregate last=oracle.query({2,3});
    require(last.sum_mod==12,"Incorrect last-element query");

    oracle.build({},Mode::exact_extrema);

    empty=oracle.query({0,0});
    require(empty.length==0,"Empty build should support empty query");
    require(empty.sum_mod==0,"Empty build should have zero sum");
}

void test_invalid_ranges(){
    ArrayOracle oracle;
    oracle.build({1,2,3},Mode::exact_extrema);

    expect_throw<std::out_of_range>(
        [&](){oracle.update({0,4},{1,1});},
        "Update with end beyond size did not throw"
    );

    expect_throw<std::out_of_range>(
        [&](){oracle.update({2,1},{1,1});},
        "Update with reversed range did not throw"
    );

    expect_throw<std::out_of_range>(
        [&](){oracle.query({0,4});},
        "Query with end beyond size did not throw"
    );

    expect_throw<std::out_of_range>(
        [&](){oracle.query({3,2});},
        "Query with reversed range did not throw"
    );

    Aggregate result=oracle.query({0,3});
    require(result.sum_mod==6,"Invalid range operation modified the array");
}

void test_invalid_exact_multiplier(){
    ArrayOracle oracle;
    oracle.build({1,2,3},Mode::exact_extrema);

    expect_throw<std::invalid_argument>(
        [&](){oracle.update({0,3},{2,5});},
        "Invalid exact-mode multiplier did not throw"
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

    require(
        result.sum_mod==
        (std::numeric_limits<Value>::max()%modulus+10)%modulus,
        "Overflowing update partially modified the array"
    );

    require(
        result.maximum.has_value() &&
        *result.maximum==std::numeric_limits<Value>::max(),
        "Overflowing update changed the maximum"
    );
}

void test_unbuilt_operations(){
    ArrayOracle oracle;

    expect_throw<std::logic_error>(
        [&](){(void)oracle.size();},
        "size() before build did not throw"
    );

    expect_throw<std::logic_error>(
        [&](){(void)oracle.mode();},
        "mode() before build did not throw"
    );

    expect_throw<std::logic_error>(
        [&](){oracle.update({0,0},{1,1});},
        "Update before build did not throw"
    );

    expect_throw<std::logic_error>(
        [&](){oracle.query({0,0});},
        "Query before build did not throw"
    );
}

void test_rebuild_and_counters(){
    ArrayOracle oracle;

    oracle.build({1,2,3},Mode::exact_extrema);
    oracle.update({0,3},{1,5});

    oracle.build({10,20},Mode::exact_extrema);

    Aggregate result=oracle.query({0,2});

    require(result.sum_mod==30,"Rebuild did not replace previous data");

    Counters stats=oracle.counters();

    require(stats.nodes_visited==0,"Oracle nodes_visited should be zero");
    require(stats.push_downs==0,"Oracle push_downs should be zero");
    require(stats.full_blocks==0,"Oracle full_blocks should be zero");
    require(stats.partial_elements==0,"Oracle partial_elements should be zero");
    require(stats.tag_compositions==0,"Oracle tag_compositions should be zero");
    require(stats.aggregate_merges==0,"Oracle aggregate_merges should be zero");

    oracle.reset_counters();

    stats=oracle.counters();

    require(stats.nodes_visited==0,"Oracle counter reset failed");
}

int main(){
    try{
        test_build_and_basic_query();
        test_exact_affine_updates();
        test_modular_mode();
        test_empty_and_boundary_ranges();
        test_invalid_ranges();
        test_invalid_exact_multiplier();
        test_overflow_is_atomic();
        test_unbuilt_operations();
        test_rebuild_and_counters();

        std::cout<<"All Oracle tests passed."<<std::endl;
        return 0;
    }
    catch(const std::exception& error){
        std::cerr<<"Oracle test failed: "<<error.what()<<std::endl;
        return 1;
    }
}
