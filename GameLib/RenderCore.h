/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "Vectors.h"

#include <numeric>
#include <string>

/*
 * Definitions of render-related types and constants that are private
 * to the rendering library but shared among the rendering compilation units.
 */

namespace Render {

//
// Shaders
//

enum class ProgramType
{
    Clouds = 0,
    GenericTextures,
    Land,
    Matte,
    MatteNDC,
    MatteWater,
    ShipRopes,
    ShipStressedSprings,
    ShipTrianglesColor,
    ShipTrianglesTexture,
    TextNDC,
    Water,

    _Last = Water
};

ProgramType ShaderFilenameToProgramType(std::string const & str);

std::string ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType
{
    AmbientLightIntensity = 0,
    MatteColor,
    OrthoMatrix,
    TextureScaling,
    WaterLevelThreshold,
    WaterTransparency
};

ProgramParameterType StrToProgramParameterType(std::string const & str);

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

enum class VertexAttributeType : GLuint
{
    //
    // Vertex attributes sourced from multiple VBO's
    //

    SharedPosition = 0,
    SharedTextureCoordinates = 1,
    Shared1XFloat = 2,


    //
    // Vertex attributes dedicated to a VBO
    //

    WaterPosition = 3,

    GenericTexturePosition = 4,
    GenericTextureCoordinates = 5,
    GenericTextureAmbientLightSensitivity = 6,

    // TODO: dedicated as long as we have one single ship and one VBO per ship
    ShipPointPosition = 7,
    ShipPointColor = 8,
    ShipPointLight = 9,
    ShipPointWater = 10,
    ShipPointTextureCoordinates = 11
};

VertexAttributeType StrToVertexAttributeType(std::string const & str);

std::string VertexAttributeTypeToStr(VertexAttributeType vertexAttribute);

struct ShaderManagerTraits
{
    using ProgramType = Render::ProgramType;
    using ProgramParameterType = Render::ProgramParameterType;
    using VertexAttributeType = Render::VertexAttributeType;

    static constexpr auto ShaderFilenameToProgramType = Render::ShaderFilenameToProgramType;
    static constexpr auto ProgramTypeToStr = Render::ProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = Render::StrToProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = Render::ProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = Render::StrToVertexAttributeType;
    static constexpr auto VertexAttributeTypeToStr = Render::VertexAttributeTypeToStr;
};

//
// Textures
//

/*
 * Describes a vertex of a texture, with all the information necessary to the shader.
 */
#pragma pack(push)
struct TextureRenderPolygonVertex
{
    vec2f position;
    vec2f textureCoordinate;

    // When 1.0, totally subject to ambient light; when 0.0, totally independent from it
    float ambientLightSensitivity;

    TextureRenderPolygonVertex(
        vec2f _position,
        vec2f _textureCoordinate,
        float _ambientLightSensitivity)
        : position(_position)
        , textureCoordinate(_textureCoordinate)
        , ambientLightSensitivity(_ambientLightSensitivity)
    {}
};
#pragma pack(pop)


//
// Text
//

/*
 * Describes a vertex of a text quad, with all the information necessary to the shader.
 */
#pragma pack(push)
struct TextQuadVertex
{
    float positionNdcX;
    float positionNdcY;
    float textureCoordinateX;
    float textureCoordinateY;
    float transparency;

    TextQuadVertex(
        float _positionNdcX,
        float _positionNdcY,
        float _textureCoordinateX,
        float _textureCoordinateY,
        float _transparency)
        : positionNdcX(_positionNdcX)
        , positionNdcY(_positionNdcY)
        , textureCoordinateX(_textureCoordinateX)
        , textureCoordinateY(_textureCoordinateY)
        , transparency(_transparency)
    {}
};
#pragma pack(pop)

}
