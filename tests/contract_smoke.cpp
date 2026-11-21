#include "cs253/lazy_segment_tree.hpp"
#include "cs253/sqrt_decomposition.hpp"
#include "cs253/array_oracle.hpp"
#include "cs253/pipeline.hpp"

#include <type_traits>
#include <stdexcept>
<<<<<<< HEAD
#include <typeinfo>
#include <vector>
#include <utility>
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::LazySegmentTree>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::SqrtDecomposition>);
static_assert(std::is_base_of_v<cs253::RangeStructure, cs253::ArrayOracle>);
static_assert(!std::is_abstract_v<cs253::LazySegmentTree>);
static_assert(!std::is_abstract_v<cs253::SqrtDecomposition>);
static_assert(!std::is_abstract_v<cs253::ArrayOracle>);
static_assert(!noexcept(std::declval<const cs253::RangeStructure&>().size()));
static_assert(!noexcept(std::declval<const cs253::RangeStructure&>().mode()));
static_assert(!noexcept(std::declval<const cs253::LazySegmentTree&>().size()));
static_assert(!noexcept(std::declval<const cs253::LazySegmentTree&>().mode()));
static_assert(!noexcept(std::declval<const cs253::SqrtDecomposition&>().size()));
static_assert(!noexcept(std::declval<const cs253::SqrtDecomposition&>().mode()));
static_assert(!noexcept(std::declval<const cs253::ArrayOracle&>().size()));
static_assert(!noexcept(std::declval<const cs253::ArrayOracle&>().mode()));
int main() {
    cs253::LazySegmentTree tree;
    cs253::SqrtDecomposition blocks;
    auto throws_logic_error = [](auto&& action) {
        try { action(); }
        catch (const std::logic_error& error) {
            return typeid(error) == typeid(std::logic_error);
        }
        catch (...) { return false; }
        return false;
    };
    if (!throws_logic_error([&] { (void)tree.size(); }) ||
        !throws_logic_error([&] { (void)tree.mode(); }) ||
        !throws_logic_error([&] { (void)blocks.size(); }) ||
        !throws_logic_error([&] { (void)blocks.mode(); })) return 1;
    for (const cs253::Range range : {cs253::Range{0, 0}, cs253::Range{0, 1}}) {
        if (!throws_logic_error([&] { tree.update(range, {1, 0}); }) ||
            !throws_logic_error([&] { (void)tree.query(range); })) return 1;
    }
    auto rejects_invalid_mode = [&] {
        try { tree.build({}, static_cast<cs253::Mode>(99)); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    if (!rejects_invalid_mode() ||
        !throws_logic_error([&] { (void)tree.size(); })) return 1;
    tree.build({}, cs253::Mode::modular_sum);
    tree.update({0, 0}, {1, 0});
    if (tree.query({0, 0}).length != 0) return 1;
    const std::vector<cs253::Value> initial{4, 7, 2};
    tree.build(initial, cs253::Mode::exact_extrema);
    blocks.build(initial, cs253::Mode::exact_extrema);
    if (!rejects_invalid_mode() || tree.query({0, 3}).sum_mod != 13) return 1;
    if (tree.size() != initial.size() || blocks.size() != initial.size() ||
        tree.mode() != cs253::Mode::exact_extrema ||
        blocks.mode() != cs253::Mode::exact_extrema) return 1;
    const cs253::AffineTag identity;
    const cs253::Aggregate empty;
    return identity.a == 1 && identity.b == 0 && empty.length == 0 &&
           empty.sum_mod == 0 && !empty.minimum && !empty.maximum ? 0 : 1;
}
=======
#include <vector>
#include <utility>
#include <iostream>

using namespace cs253;

static_assert(std::is_base_of_v<RangeStructure,LazySegmentTree>);
static_assert(std::is_base_of_v<RangeStructure,SqrtDecomposition>);
static_assert(std::is_base_of_v<RangeStructure,ArrayOracle>);

static_assert(!std::is_abstract_v<LazySegmentTree>);
static_assert(!std::is_abstract_v<SqrtDecomposition>);
static_assert(!std::is_abstract_v<ArrayOracle>);

static_assert(!noexcept(std::declval<const RangeStructure&>().size()));
static_assert(!noexcept(std::declval<const RangeStructure&>().mode()));

static_assert(!noexcept(std::declval<const LazySegmentTree&>().size()));
static_assert(!noexcept(std::declval<const LazySegmentTree&>().mode()));

static_assert(!noexcept(std::declval<const SqrtDecomposition&>().size()));
static_assert(!noexcept(std::declval<const SqrtDecomposition&>().mode()));

static_assert(!noexcept(std::declval<const ArrayOracle&>().size()));
static_assert(!noexcept(std::declval<const ArrayOracle&>().mode()));

int main(){
    LazySegmentTree tree;
    SqrtDecomposition blocks;
    ArrayOracle oracle;

    auto throws_logic_error=[](auto&& action){
        try{
            action();
        }
        catch(const std::logic_error&){
            return true;
        }
        catch(...){
            return false;
        }
        return false;
    };

    // Pre-build contract checks.
    if(!throws_logic_error([&]{(void)tree.size();})){
        std::cerr<<"FAIL: tree.size() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)tree.mode();})){
        std::cerr<<"FAIL: tree.mode() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)blocks.size();})){
        std::cerr<<"FAIL: blocks.size() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)blocks.mode();})){
        std::cerr<<"FAIL: blocks.mode() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)oracle.size();})){
        std::cerr<<"FAIL: oracle.size() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)oracle.mode();})){
        std::cerr<<"FAIL: oracle.mode() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{tree.update({0,0},{1,0});})){
        std::cerr<<"FAIL: tree.update() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)tree.query({0,0});})){
        std::cerr<<"FAIL: tree.query() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{blocks.update({0,0},{1,0});})){
        std::cerr<<"FAIL: blocks.update() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)blocks.query({0,0});})){
        std::cerr<<"FAIL: blocks.query() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{oracle.update({0,0},{1,0});})){
        std::cerr<<"FAIL: oracle.update() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    if(!throws_logic_error([&]{(void)oracle.query({0,0});})){
        std::cerr<<"FAIL: oracle.query() before build did not throw logic_error"<<std::endl;
        return 1;
    }

    // Empty build contract.
    tree.build({},Mode::exact_extrema);
    blocks.build({},Mode::exact_extrema);
    oracle.build({},Mode::exact_extrema);

    auto check_empty=[](RangeStructure& structure){
        auto result=structure.query({0,0});

        return result.length==0 &&
            result.sum_mod==0 &&
            !result.minimum &&
            !result.maximum;
    };

    if(!check_empty(tree)){
        std::cerr<<"FAIL: empty tree query returned incorrect aggregate"<<std::endl;
        return 1;
    }

    if(!check_empty(blocks)){
        std::cerr<<"FAIL: empty blocks query returned incorrect aggregate"<<std::endl;
        return 1;
    }

    if(!check_empty(oracle)){
        std::cerr<<"FAIL: empty oracle query returned incorrect aggregate"<<std::endl;
        return 1;
    }

    // Updates on an empty structure must not change it.
    tree.update({0,0},{1,5});
    blocks.update({0,0},{1,5});
    oracle.update({0,0},{1,5});

    if(!check_empty(tree)){
        std::cerr<<"FAIL: empty tree changed after update"<<std::endl;
        return 1;
    }

    if(!check_empty(blocks)){
        std::cerr<<"FAIL: empty blocks changed after update"<<std::endl;
        return 1;
    }

    if(!check_empty(oracle)){
        std::cerr<<"FAIL: empty oracle changed after update"<<std::endl;
        return 1;
    }

    // Normal build contract.
    const std::vector<Value> initial{4,7,2};

    tree.build(initial,Mode::exact_extrema);
    blocks.build(initial,Mode::exact_extrema);
    oracle.build(initial,Mode::exact_extrema);

    if(tree.size()!=initial.size()){
        std::cerr<<"FAIL: tree.size() incorrect after build"<<std::endl;
        return 1;
    }

    if(blocks.size()!=initial.size()){
        std::cerr<<"FAIL: blocks.size() incorrect after build"<<std::endl;
        return 1;
    }

    if(oracle.size()!=initial.size()){
        std::cerr<<"FAIL: oracle.size() incorrect after build"<<std::endl;
        return 1;
    }

    if(tree.mode()!=Mode::exact_extrema){
        std::cerr<<"FAIL: tree.mode() incorrect after build"<<std::endl;
        return 1;
    }

    if(blocks.mode()!=Mode::exact_extrema){
        std::cerr<<"FAIL: blocks.mode() incorrect after build"<<std::endl;
        return 1;
    }

    if(oracle.mode()!=Mode::exact_extrema){
        std::cerr<<"FAIL: oracle.mode() incorrect after build"<<std::endl;
        return 1;
    }

    const AffineTag identity;
    const Aggregate empty;

    if(identity.a!=1 || identity.b!=0){
        std::cerr<<"FAIL: default AffineTag is not identity"<<std::endl;
        return 1;
    }

    if(empty.length!=0 ||
       empty.sum_mod!=0 ||
       empty.minimum ||
       empty.maximum){
        std::cerr<<"FAIL: default Aggregate is not empty"<<std::endl;
        return 1;
    }

    std::cout<<"contract_smoke: all checks passed"<<std::endl;

    return 0;
}
>>>>>>> 62ed4d2 (Implement Oracle and SqrtDecomposition)
