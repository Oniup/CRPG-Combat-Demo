#pragma once

#include "engine/core/Error.h"
#include "engine/utilities/TypeInfo.h"
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace eng::intl {

enum class Header;

} // namespace eng::intl
namespace eng {

// Defines the uniquely defined fields for specific ID in file:
// `B = "Barrel" | health = 50, element = "fire"`
// - `B` is the key to access this data from the LevelData struct
// - `"Barrel"` is the actor class
// - `health = 50, ...` remaining fields are pushed to the dictionary stored as fields
//
// Note: StringHash and std::equal_to<> for unordered_map is due to C++ now allowing lookup using
// std::string_view when the key is std::string for some reason.
// Found solution from: https://en.cppreference.com/w/cpp/container/unordered_map/find.html
struct LevelTypeDef
{
    // TODO: Add Vector2
    using FieldVariant = std::variant<int, float, bool, char, std::string>;
    using UnorderedMap = std::unordered_map<std::string, FieldVariant, StringHash, std::equal_to<>>;

    std::string name;    // Actor class name (NEEDS TO MATCH WITH THE TYPE)
    unsigned hash;       // Hash Id of actor class name
    UnorderedMap fields; // Data passed into newly created actor in which it can use to initialize

    template <typename T>
    const T& GetField(const std::string_view& fieldName) const
    {
        const auto iter = fields.find(fieldName);
        ASSERT(iter != fields.end() && std::holds_alternative<T>(iter->second),
               "Failed to find {} field, make sure that definition for {} contains this field",
               fieldName,
               name);

        return std::get<T>(iter->second);
    }

    template <typename T>
    T GetField(const std::string_view& fieldName, T defaultValue) const
    {
        const auto iter = fields.find(fieldName);
        if (iter != fields.end() && std::holds_alternative<T>(iter->second))
            return std::get<T>(iter->second);

        return defaultValue;
    }

    // Obtains field from `fields` dictionary, if doesn't exist or incorrect type; return nullptr
    template <typename T>
    const T* TryGetField(const std::string_view& fieldName) const
    {
        const auto iter = fields.find(fieldName);
        if (iter != fields.end())
        {
            const T* result = std::get_if<T>(&iter->second);
            if (result)
                return result;
        }

        return nullptr;
    }
};

// Defines the position of actors and terrain tiles in the grid.
struct Layer
{
    struct Location
    {
        unsigned x;  // Column grid position
        unsigned z;  // Row grid position
        unsigned id; // Id used as a key to access `DefinitionFields`
    };

    unsigned yIndex;               // Height position relative to other layers
    std::vector<unsigned> terrain; // Static tile positions (Tilemap)
    std::vector<Location> actors;  // Dynamic/Static actors (Barrel, Ladder, Archer)
};

// [Meta]
// name = "Test Level"
// width = 5
// height = 5
// cellWidth = 1.0
// cellHeight = 1.0
//
// # Name = "Path"
// [Resources]
//
// # ID = "TileName" | field=value, field=value, ...
// [TerrainDefs]
// + = "Floor"     | tint = "c8bfba", walkable = true, height = 0.5
// W = "HighWall"  | tint = "515459", height = 3.0
// w = "ShortWall" | tint = "515459", height = 1.0
// v = "Stairs"    | tint = "c8bfba", walkable = true, slope = 's'
//
// # ID = "ActorClass" | field=value, field=value, ...
// [ActorDefs]
// B = "Barrel" | health = 50, element = "fire"
// 0 = "Ladder" | toLayer = 0
// 1 = "Ladder" | toLayer = 1
//
// # Lowest layer specified by the :0
// [Terrain:0]
// . + + + .
// . + v + .
// + + + + +
// + W w W +
// + + + + +
// [Actors:0]
// . . . . .
// . . . . .
// . . . . B
// . . . . .
// . . 1 . .
//
// # Layer above previous specified by the :1
// # Left cell (1, 3) and (3, 3) empty as HighWall from lower layer will extrude through so should
// # leave empty
// [Terrain:1]
// . . . . .
// . . . . .
// . + + + .
// . . + . .
// . + . + .
// [Actors:1]
// . . . . .
// . . . . .
// . . . B .
// . . . . .
// . . 0 . .
struct LevelData
{
    std::string name;                                            // Title of level
    unsigned width;                                              // Number of columns tiles
    unsigned length;                                             // Number of row tiles
    float cellWidth;                                             // Width of each tile
    float cellHeight;                                            // Height of each tile
    std::string tilemapClassName = "eng::Tilemap";               // Tilemap class name
    std::vector<Layer> layers;                                   // Layers that make up the world
    std::unordered_map<unsigned, LevelTypeDef> tileDefinitions;  // Unique Terrain ID fields
    std::unordered_map<unsigned, LevelTypeDef> actorDefinitions; // Unique Actor ID fields

    LevelData(const std::string_view& path);

private:
    // Reads if the line is representing a header in format `[HEADER_NAME]`. If no
    // header is found, then return `intl::Header::NONE`.
    static intl::Header ReadHeader(const std::string_view& line, unsigned& zIndex,
                                   unsigned lineNumber);

    // Reads and sets fields related to the meta data: name, width, height, and
    // layer_count
    void ReadMetaData(const std::string_view& line, unsigned lineNumber);

    // Reads the actor/tile's fields and pushes it into the dictionary using the
    // field name as the UUID (key) to access the dictionary.
    void ReadDefinitions(std::unordered_map<unsigned, LevelTypeDef>& definitions,
                         const std::string_view& line, unsigned lineNumber);

    // Reads the map location of a specific actor. The specific type can be looked up in the
    // respective definition dictionary.
    void ReadActorLocation(const std::string_view& line, unsigned& zIndex, unsigned lineNumber);

    // Reads the map location of a specific tile for terrain. The specific type can be looked up in
    // the respective definition dictionary.
    void ReadTerrainLocation(const std::string_view& line, unsigned& zIndex, unsigned lineNumber);

    // Reads through all fields associated to an actor/tile and returns a vector
    // containing all converted field data, alongside their name
    void ReadFieldArrayData(const std::string_view& fieldArrayString,
                            LevelTypeDef::UnorderedMap& fields, unsigned lineNumber);

    // Converts the field value string into correct type, returning a variant.
    LevelTypeDef::FieldVariant ReadDataField(const std::string_view& field_value,
                                             unsigned lineNumber) const;

    // Separates "field_name = value" into a tuple[field, value]
    std::tuple<std::string_view, std::string_view> GetFieldEntry(const std::string_view& line,
                                                                 unsigned lineNumber) const;
};

} // namespace eng
