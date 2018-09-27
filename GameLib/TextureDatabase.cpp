/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextureDatabase.h"

#include "GameException.h"
#include "ResourceLoader.h"
#include "Utils.h"

#include <map>
#include <regex>

TextureGroupType StrToTextureGroupType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "cloud")
        return TextureGroupType::Cloud;
    else if (lstr == "land")
        return TextureGroupType::Land;
    else if (lstr == "pinned_point")
        return TextureGroupType::PinnedPoint;
    else if (lstr == "rc_bomb")
        return TextureGroupType::RcBomb;
    else if (lstr == "timer_bomb")
        return TextureGroupType::TimerBomb;
    else if (lstr == "water")
        return TextureGroupType::Water;
    else
        throw GameException("Unrecognized TextureGroupType \"" + str + "\"");
}

TextureFrame TextureGroup::GetFrame(TextureFrameIndex frameIndex) const
{
    assert(frameIndex < mFrameSpecifications.size());

    ImageData imageData = ResourceLoader::LoadImageRgbaLowerLeft(mFrameSpecifications[frameIndex].FilePath);

    return TextureFrame(
        mFrameSpecifications[frameIndex].Metadata,
        std::move(imageData.Data));
}

TextureDatabase TextureDatabase::Load(
    std::filesystem::path const & texturesRoot,
    ProgressCallback progressCallback)
{
    //
    // Visit directory and build set of all files
    //

    struct FileData
    {
        std::filesystem::path Path;
        std::string Stem;

        FileData(
            std::filesystem::path path,
            std::string stem)
            : Path(path)
            , Stem(stem)
        {}
    };

    std::vector<FileData> allTextureFiles;

    for (auto const & entryIt : std::filesystem::directory_iterator(texturesRoot))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension().string() != "json")
        {
            std::string const stem = entryIt.path().filename().stem().string();

            allTextureFiles.emplace_back(
                entryIt.path(),
                stem);
        }
    }


    //
    // Load JSON file
    //

    std::filesystem::path const jsonFilePath = texturesRoot / "textures.json";
    picojson::value root = Utils::ParseJSONFile(jsonFilePath.string());
    if (!root.is<picojson::array>())
    {
        throw GameException("Texture database: file \"" + jsonFilePath.string() + "\" does not contain a JSON array");
    }


    //
    // Process JSON groups and build texture groups
    //

    std::vector<TextureGroup> textureGroups;

    for (auto const & groupValue : root.get<picojson::array>())
    {
        if (!groupValue.is<picojson::object>())
        {
            throw GameException("Texture database: found a non-object group in database");
        }

        auto groupJson = groupValue.get<picojson::object>();

        std::string groupName = Utils::GetMandatoryJsonMember<std::string>(groupJson, "groupName");
        TextureGroupType groupType = StrToTextureGroupType(groupName);
        
        // Load defaults
        std::optional<float> groupWorldScaling = Utils::GetOptionalJsonMember<float>(groupJson, "worldScaling");
        bool groupHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(groupJson, "hasOwnAmbientLight", false);
        int groupAnchorX = Utils::GetOptionalJsonMember<int>(groupJson, "anchorX", 0);
        int groupAnchorY = Utils::GetOptionalJsonMember<int>(groupJson, "anchorY", 0);


        //
        // Process frames from JSON and build texture frames
        //

        std::vector<TextureFrameSpecification> textureFrames;        
        
        for (auto const & frameValue : Utils::GetMandatoryJsonArray(groupJson, "frames"))
        {
            if (!frameValue.is<picojson::object>())
            {
                throw GameException("Texture database: found a non-object frame in database");
            }

            auto frameJson = frameValue.get<picojson::object>();

            // Get frame properties
            std::optional<float> frameWorldScaling = Utils::GetOptionalJsonMember<float>(frameJson, "worldScaling");
            std::optional<bool> frameHasOwnAmbientLight = Utils::GetOptionalJsonMember<bool>(frameJson, "hasOwnAmbientLight");
            std::optional<int> frameAnchorX = Utils::GetOptionalJsonMember<int>(frameJson, "anchorX");
            std::optional<int> frameAnchorY = Utils::GetOptionalJsonMember<int>(frameJson, "anchorY");

            // Get filename and make regex out of it
            std::string frameFilename = Utils::GetMandatoryJsonMember<std::string>(frameJson, "filename");
            std::regex const frameFilenameRegex("^" + frameFilename + "$");

            // Find files matching the regex
            int filesFoundCount = 0;
            for (auto fileIt = allTextureFiles.begin(); fileIt!= allTextureFiles.end(); /* incremented in loop */)
            {
                FileData const & fileData = *fileIt;

                if (std::regex_match(fileData.Stem, frameFilenameRegex))
                {
                    // This file belongs to this group

                    //
                    // Extract frame index
                    //

                    static std::regex const TextureFilenameFrameIndexRegex("^.+?_(\\d+)$");
                    std::smatch frameIndexMatch;
                    if (!std::regex_match(fileData.Stem, frameIndexMatch, TextureFilenameFrameIndexRegex))
                    {
                        throw GameException("Texture database: cannot find frame index in texture filename \"" + fileData.Stem + "\"");
                    }

                    assert(frameIndexMatch.size() == 2);
                    TextureFrameIndex frameIndex = static_cast<TextureFrameIndex>(std::stoi(frameIndexMatch[1].str()));


                    //
                    // Resolve properties
                    //

                    if (!groupWorldScaling && !frameWorldScaling)
                    { 
                        throw GameException("Texture database: cannot find \"worldScaling\" property in frame \"" + frameFilename + "\"");
                    }

                    float worldScaling = !!frameWorldScaling ? *frameWorldScaling : *groupWorldScaling;
                    bool hasOwnAmbientLight = !!frameHasOwnAmbientLight ? *frameHasOwnAmbientLight : groupHasOwnAmbientLight;
                    int anchorX = !!frameAnchorX ? *frameAnchorX : groupAnchorX;
                    int anchorY = !!frameAnchorY ? *frameAnchorY : groupAnchorY;

                    //
                    // Get texture size and calculate world dimensions
                    //

                    ImageSize textureSize = ResourceLoader::GetImageSize(fileData.Path);
                    // TODO: verify clouds look ok
                    float worldWidth = static_cast<float>(textureSize.Width) * worldScaling;
                    float worldHeight = static_cast<float>(textureSize.Height) * worldScaling;

                    //
                    // Store frame
                    //

                    textureFrames.emplace_back(
                        TextureFrameSpecification(
                            TextureFrameMetadata(
                                textureSize,
                                worldWidth,
                                worldHeight,
                                hasOwnAmbientLight,
                                anchorX,
                                anchorY,
                                TextureFrameId(groupType, frameIndex)),
                            fileData.Path));

                    //
                    // Remove this file from the list
                    //

                    fileIt = allTextureFiles.erase(fileIt);

                    ++filesFoundCount;
                }
                else
                {
                    // Advance
                    ++fileIt;
                }
            }

            // Make sure at least one matching file was found for this frame specification
            if (0 == filesFoundCount)
            {
                throw GameException("Texture database: couldn't match any file to frame file \"" + frameFilename + "\"");
            }
        }

        // Sort frames by frame index
        std::sort(
            textureFrames.begin(),
            textureFrames.end(),
            [](auto const & a, auto const & b)
            {
                return a.Metadata.FrameId.FrameIndex < b.Metadata.FrameId.FrameIndex;
            });

        // Make sure all indices are found
        for (size_t expectedIndex = 0; expectedIndex < textureFrames.size(); ++expectedIndex)
        {
            if (textureFrames[expectedIndex].Metadata.FrameId.FrameIndex < expectedIndex)
            {
                throw GameException("Texture database: duplicate frame \"" + std::to_string(textureFrames[expectedIndex].Metadata.FrameId.FrameIndex) + "\" in group \"" + groupName + "\"");
            }
            else if (textureFrames[expectedIndex].Metadata.FrameId.FrameIndex > expectedIndex)
            {
                throw GameException("Texture database: missing frame \"" + std::to_string(expectedIndex) + "\" in group \"" + groupName + "\"");
            }
        }

        // Store texture frames
        textureGroups.emplace_back(textureFrames);
    }

    // Make sure all textures found in file system have been exhausted
    if (!allTextureFiles.empty())
    {
        throw GameException("Texture database: couldn't match " + std::to_string(allTextureFiles.size()) + " texture files (e.g. \"" + allTextureFiles[0].Stem + "\") to texture specification file");
    }

    return TextureDatabase(std::move(textureGroups));
}
