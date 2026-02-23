#pragma once

#include "types.hpp"

#include <aglio/packager.hpp>

namespace Test::packager {

struct MyCrc {
    using type = std::uint32_t;

    static type calc(std::span<std::byte const> data) {
        // A dummy CRC function for testing
        type crc = 0;
        for(auto b : data) { crc += static_cast<type>(b); }
        return crc;
    }
};

namespace Configs {

    // Minimal (Size_t only, no CRC, no PackageStart)
    struct Minimal {
        using Size_t = std::uint32_t;
    };

    // PackageStart only (no CRC)
    struct SimplePackageStart {
        using Size_t                                = std::uint32_t;
        static constexpr std::uint16_t PackageStart = 0xABCD;
    };

    // CRC only (UseHeaderCrc defaults to true when CRC present)
    struct SimpleCrc {
        using Crc    = MyCrc;
        using Size_t = std::uint32_t;
    };

    // CRC with UseHeaderCrc explicitly disabled
    struct CrcNoHeader {
        using Crc                          = MyCrc;
        using Size_t                       = std::uint32_t;
        static constexpr bool UseHeaderCrc = false;
    };

    // PackageStart + CRC (implicit UseHeaderCrc=true)
    struct Full {
        using Crc                                   = MyCrc;
        using Size_t                                = std::uint32_t;
        static constexpr std::uint16_t PackageStart = 0xABCD;
    };

    // PackageStart + CRC with UseHeaderCrc=false
    struct FullNoHeaderCrc {
        using Crc                                   = MyCrc;
        using Size_t                                = std::uint32_t;
        static constexpr std::uint16_t PackageStart = 0xABCD;
        static constexpr bool          UseHeaderCrc = false;
    };

}   // namespace Configs

template<typename T, typename TTuple>
struct product_one_trait;

template<typename T, typename... Us>
struct product_one_trait<T, std::tuple<Us...>> {
    using type = std::tuple<std::tuple<T, Us>...>;
};

template<typename TTuple1, typename TTuple2>
struct cartesian_product;

template<typename... Ts, typename... Us>
struct cartesian_product<std::tuple<Ts...>, std::tuple<Us...>> {
    using type = decltype(std::tuple_cat(
      std::declval<typename product_one_trait<Ts, std::tuple<Us...>>::type>()...));
};

using ConfigsList = std::tuple<Configs::Minimal,
                               Configs::SimplePackageStart,
                               Configs::SimpleCrc,
                               Configs::CrcNoHeader,
                               Configs::Full,
                               Configs::FullNoHeaderCrc>;

using TestCases = typename cartesian_product<Types::List, ConfigsList>::type;

template<typename Type,
         typename Packager>
void test() {
    std::vector<std::byte> buffer{};

    Type t_in = Types::createDefault<Type>();

    Packager::pack(buffer, t_in);
    Type t_out{};
    auto result = Packager::unpack(buffer, t_out);

    REQUIRE(result.has_value());
    CHECK(buffer.size() == *result);
    CHECK(t_in == t_out);
}
}   // namespace Test::packager

TEMPLATE_LIST_TEST_CASE("Packager",
                        "[cartesian]",
                        Test::packager::TestCases) {
    using Type   = std::tuple_element_t<0, TestType>;
    using Config = std::tuple_element_t<1, TestType>;

    Test::packager::test<Type, aglio::Packager<Config>>();
}

TEST_CASE("Serializer rejects range that exceeds fixed-capacity container max_size",
          "[serializer]") {
    using Packager = aglio::Packager<Test::packager::Configs::Minimal>;

    // Serialize a vector with more elements than the target array can hold
    std::vector<int>       src    = {1, 2, 3, 4, 5};
    std::vector<std::byte> buffer = {};
    Packager::pack(buffer, src);

    // Deserialize into a smaller array — must fail because 5 > max_size() == 3
    std::array<int, 3> dst{};
    auto               result = Packager::unpack(buffer, dst);
    CHECK(!result.has_value());
}

TEST_CASE("Packager pair<primitive, struct ref>",
          "[packager]") {
    using Packager = aglio::Packager<Test::packager::Configs::Minimal>;

    Types::Primitive                  prim_in = Types::createDefault<Types::Primitive>();
    std::pair<int, Types::Primitive&> pair_in{42, prim_in};

    std::vector<std::byte> buffer{};
    Packager::pack(buffer, pair_in);

    Types::Primitive                  prim_out{};
    std::pair<int, Types::Primitive&> pair_out{0, prim_out};

    auto result = Packager::unpack(buffer, pair_out);

    REQUIRE(result.has_value());
    CHECK(buffer.size() == *result);
    CHECK(pair_in.first == pair_out.first);
    CHECK(pair_in.second == pair_out.second);
}

TEST_CASE("Packager pair<primitive, variant<structs> ref>",
          "[packager]") {
    using Packager = aglio::Packager<Test::packager::Configs::Minimal>;
    using Variant  = std::variant<Types::Primitive, Types::Container, Types::Enum>;

    SECTION("Primitive variant") {
        Variant                  var_in = Types::createDefault<Types::Primitive>();
        std::pair<int, Variant&> pair_in{1, var_in};

        std::vector<std::byte> buffer{};
        Packager::pack(buffer, pair_in);

        Variant                  var_out{};
        std::pair<int, Variant&> pair_out{0, var_out};

        auto result = Packager::unpack(buffer, pair_out);

        REQUIRE(result.has_value());
        CHECK(buffer.size() == *result);
        CHECK(pair_in.first == pair_out.first);
        CHECK(pair_in.second == pair_out.second);
    }

    SECTION("Container variant") {
        Variant                  var_in = Types::createDefault<Types::Container>();
        std::pair<int, Variant&> pair_in{2, var_in};

        std::vector<std::byte> buffer{};
        Packager::pack(buffer, pair_in);

        Variant                  var_out{};
        std::pair<int, Variant&> pair_out{0, var_out};

        auto result = Packager::unpack(buffer, pair_out);

        REQUIRE(result.has_value());
        CHECK(buffer.size() == *result);
        CHECK(pair_in.first == pair_out.first);
        CHECK(pair_in.second == pair_out.second);
    }

    SECTION("Enum variant") {
        Variant                  var_in = Types::createDefault<Types::Enum>();
        std::pair<int, Variant&> pair_in{3, var_in};

        std::vector<std::byte> buffer{};
        Packager::pack(buffer, pair_in);

        Variant                  var_out{};
        std::pair<int, Variant&> pair_out{0, var_out};

        auto result = Packager::unpack(buffer, pair_out);

        REQUIRE(result.has_value());
        CHECK(buffer.size() == *result);
        CHECK(pair_in.first == pair_out.first);
        CHECK(pair_in.second == pair_out.second);
    }
}
