#include <sstream>
#include <algorithm>
#include <map>
#include <vector>
#include <memory>

#include <REDObject.h>
#include <REDVector3.h>
#include <REDIResourceManager.h>
#include <REDImageTools.h>
#include <REDIImage2D.h>
#include <REDFactory.h>
#include <REDITransformShape.h>
#include <REDIShape.h>

#include <hoops_luminate_bridge/Hoops3DFLuminateBridge.h>
#include <hoops_luminate_bridge/LuminateRCTest.h>

#define PBR_TEXTURE_SIZE 1024

constexpr char const* CHANNEL_DIFFUSE_COLOR = "diffuse color";
constexpr char const* CHANNEL_TRANSMISSION_COLOR = "transmission";
constexpr char const* CHANNEL_DIFFUSE_TEXTURE = "diffuse texture";
constexpr char const* CHANNEL_BUMP_TEXTURE = "bump";
constexpr char const* CHANNEL_ENV_TEXTURE = "environment";

namespace hoops_luminate_bridge {

    LuminateBridge3DF::LuminateBridge3DF(HBaseView* a_3dfView): m_3dfView(a_3dfView) {}

    LuminateBridge3DF::~LuminateBridge3DF() {}

    void LuminateBridge3DF::saveCameraState() { m_3dfView->GetCamera(&m_3dfCamera); }

    LuminateSceneInfoPtr LuminateBridge3DF::convertScene() { return convert3DFSceneToLuminate(m_3dfView); }

    bool LuminateBridge3DF::checkCameraChange()
    {
        HCamera camera;
        camera.GetFromView(m_3dfView);

        bool hasChanged = m_3dfCamera.CameraDifferent(camera);

        if (hasChanged)
            m_3dfCamera = camera;

        return hasChanged;
    }

    CameraInfo LuminateBridge3DF::getCameraInfo() { return get3DFCameraInfo(m_3dfView); }

    void LuminateBridge3DF::updateSelectedSegmentTransform()
    {
        ConversionContext3DF* conversionData3DF = (ConversionContext3DF*)m_conversionDataPtr.get();
        Selected3DFSegmentInfoPtr selected3DFSegment = std::dynamic_pointer_cast<Selected3DFSegmentInfo>(m_selectedSegment);

        if (selected3DFSegment == nullptr)
            return;

        hoops_luminate_bridge::SegmentTransformShapeMap::iterator it =
            conversionData3DF->segmentTransformShapeMap.find(selected3DFSegment->m_serializedKeyPath);

        if (it != conversionData3DF->segmentTransformShapeMap.end()) {

            HC_Open_Segment_By_Key(selected3DFSegment->m_segmentKey);
            if (HC_Show_Existence("modelling matrix")) {
                float vizMatrix[16];
                HC_Show_Modelling_Matrix(vizMatrix);

                RED::Matrix redMatrix = RED::Matrix::IDENTITY;
                redMatrix.SetColumnMajorMatrix(vizMatrix);

                RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
                RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();
                RED::ITransformShape* itransform = it->second->As<RED::ITransformShape>();
                itransform->SetMatrix(&redMatrix, iresourceManager->GetState());

                resetFrame();
            }
            HC_Close_Segment();
        }

        m_selectedSegmentTransformIsDirty = false;
    }

    std::vector<RED::Object*> LuminateBridge3DF::getSelectedLuminateMeshShapes()
    {
        if (m_selectedSegment != nullptr) {
            ConversionContext3DF* conversionData3DF = (ConversionContext3DF*)m_conversionDataPtr.get();
            Selected3DFSegmentInfoPtr selected3DFSegment = std::dynamic_pointer_cast<Selected3DFSegmentInfo>(m_selectedSegment);

            hoops_luminate_bridge::SegmentMeshShapesMap::iterator it =
                conversionData3DF->segmentMeshShapesMap.find(selected3DFSegment->m_segmentKey);

            if (it != conversionData3DF->segmentMeshShapesMap.end()) {
                return it->second;
            }
        }
        return std::vector<RED::Object*>();
    }

    RED::Object* LuminateBridge3DF::getSelectedLuminateTransformNode()
    {
        if (m_selectedSegment != nullptr) {
            ConversionContext3DF* conversionData3DF = (ConversionContext3DF*)m_conversionDataPtr.get();
            Selected3DFSegmentInfoPtr selected3DFSegment = std::dynamic_pointer_cast<Selected3DFSegmentInfo>(m_selectedSegment);

            hoops_luminate_bridge::SegmentTransformShapeMap::iterator it =
                conversionData3DF->segmentTransformShapeMap.find(selected3DFSegment->m_serializedKeyPath);

            if (it != conversionData3DF->segmentTransformShapeMap.end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    RED_RC LuminateBridge3DF::syncRootTransform()
    {
        HC_KEY modelRootSegmentKey = m_3dfView->GetModelKey();
        HC_Open_Segment_By_Key(modelRootSegmentKey);
        
        float visMatrix[16];

        if (HC_Show_Existence("modelling matrix")) {
            HC_Show_Modelling_Matrix(visMatrix);

            RED::Matrix redMatrix = RED::Matrix::IDENTITY; 
            redMatrix.SetColumnMajorMatrix(visMatrix);

            // Restore left handedness transform.
            if (m_conversionDataPtr->viewHandedness == Handedness::LeftHanded) {
                redMatrix = HoopsLuminateBridge::s_leftHandedToRightHandedMatrix * redMatrix;
            }

            RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
            RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();
            
            LuminateSceneInfoPtr sceneInfo = m_conversionDataPtr;
            RED::ITransformShape* itransform = sceneInfo->rootTransformShape->As<RED::ITransformShape>();
            RC_CHECK(itransform->SetMatrix(&redMatrix, iresourceManager->GetState()));
        
            resetFrame();
        }
        
        HC_Close_Segment();
        m_rootTransformIsDirty = false;

        return RED_RC();
    }

    CameraInfo get3DFCameraInfo(HBaseView* a_3dfView)
    {
        //////////////////////////////////////////
        // Get the 3DF camera.
        //////////////////////////////////////////

        HCamera camera;
        camera.GetFromView(a_3dfView);

        //////////////////////////////////////////
        // Compute camera axis in world space.
        //////////////////////////////////////////

        RED::Vector3 eyePosition, right, top, sight, target;

        target = RED::Vector3(camera.target.x, camera.target.y, camera.target.z);
        top = RED::Vector3(camera.up_vector.x, camera.up_vector.y, camera.up_vector.z);
        eyePosition = RED::Vector3(camera.position.x, camera.position.y, camera.position.z);

        sight = target - eyePosition;
        sight.Normalize();

        right = top.Cross2(sight);
        right.Normalize();

        //////////////////////////////////////////
        // Get camera projection mode
        //////////////////////////////////////////

        ProjectionMode projectionMode = ProjectionMode::Perspective;
        if (std::string(camera.projection) == "perspective")
            projectionMode = ProjectionMode::Perspective;
        else if (std::string(camera.projection) == "orthographic")
            projectionMode = ProjectionMode::Orthographic;
        else if (std::string(camera.projection) == "stretched")
            projectionMode = ProjectionMode::Stretched;

        //////////////////////////////////////////
        // Return result.
        //////////////////////////////////////////

        CameraInfo cameraInfo;
        cameraInfo.projectionMode = projectionMode;
        cameraInfo.target = target;
        cameraInfo.top = top;
        cameraInfo.eyePosition = eyePosition;
        cameraInfo.sight = sight;
        cameraInfo.right = right;
        cameraInfo.field_width = camera.field_width;
        cameraInfo.field_height = camera.field_height;

        return cameraInfo;
    }

    Handedness getViewHandedness(HBaseView* a_baseView)
    {
        // 3DF default is left handedness.
        Handedness handedness = Handedness::LeftHanded;

        HC_Open_Segment_By_Key(a_baseView->GetSceneKey());
        {
            if (HC_Show_Existence("handedness")) {
                char handednessStr[256];
                HC_Show_Handedness(handednessStr);

                if (streq(handednessStr, "left"))
                    handedness = Handedness::LeftHanded;
                else if (streq(handednessStr, "right"))
                    handedness = Handedness::RightHanded;
            }
        }
        HC_Close_Segment();

        return handedness;
    }

    bool isSegmentVisible(std::vector<HC_KEY> const& a_segmentKeyPath)
    {
        char visibility_buffer[1024];
        HC_PShow_Net_Visibility((int)a_segmentKeyPath.size(), &a_segmentKeyPath[0], visibility_buffer);

        // We check different ways for visibility to be off

        std::stringstream visibilityOptionsStream(visibility_buffer);
        std::string visibilityOption;
        bool facesAreVisible = true;

        while (facesAreVisible && std::getline(visibilityOptionsStream, visibilityOption, ',')) {
            facesAreVisible = visibilityOption != "off" && visibilityOption != "faces=off" &&
                              visibilityOption != "geometry=off" && visibilityOption != "polygon=off";
        }

        return facesAreVisible;
    }

    ImageData getImageData(HC_KEY a_imageKey)
    {
        char imageName[1024] = "";
        HC_Show_Image_Name(a_imageKey, imageName);

        float centerX, centerY, centerZ;
        char format[1024] = "";
        int width, height;
        // NOTE: HC_Show_Image_Format returns the same 'format' value (ex. 'rgb,name=brick')
        HC_Show_Image_Size(a_imageKey, &centerX, &centerY, &centerZ, format, &width, &height);

        int bytesPerPixel = HC_Show_Image_Bytes_Per_Pixel(a_imageKey);

        // Get the 3DF image pixels

        unsigned char* pixelData = new unsigned char[width * height * bytesPerPixel];
        memset(pixelData, 0, width * height * bytesPerPixel);
        HC_Show_Image(a_imageKey, 0, 0, 0, format, &width, &height, pixelData);

        // Register the image data

        ImageData imageData;
        imageData.name = imageName;
        imageData.width = width;
        imageData.height = height;
        imageData.bytesPerPixel = bytesPerPixel;
        imageData.pixelData = pixelData;

        return imageData;
    }

    bool getSegmentTextureImageData(std::vector<HC_KEY> const& a_segmentKeyPath,
                                    std::string const& a_textureName,
                                    ImageData& a_outImageData)
    {
        HC_KEY imageKey;
        bool success = false;

        char textureDefinition[1024];
        if (HC_PShow_Net_Texture(
                (int)a_segmentKeyPath.size(), &a_segmentKeyPath[0], a_textureName.c_str(), textureDefinition, &imageKey) &&
            imageKey != HC_ERROR_KEY) {
            a_outImageData = getImageData(imageKey);
            success = true;
        }

        return success;
    }

    RED::Object* getSegmentTextureRedImage(std::vector<HC_KEY> const& a_segmentKeyPath,
                                           std::string const& a_textureName,
                                           RED::Object* a_resourceManager)
    {
        if (a_textureName.empty())
            return nullptr;

        ImageData imageData;
        RED::Object* redImage = nullptr;

        if (getSegmentTextureImageData(a_segmentKeyPath, a_textureName, imageData)) {
            redImage = createREDIImage2D(imageData, a_resourceManager);
        }

        return redImage;
    }

    bool registerTexture(std::vector<HC_KEY> const& a_segmentKeyPath,
                         std::string const& a_textureName,
                         RED::Object* a_resourceManager,
                         ImageNameToLuminateMap& a_ioImageToLuminateMap,
                         TextureNameImageNameMap& a_ioTextureNameImageNameMap,
                         RED::Object*& a_outRedImage)
    {
        a_outRedImage = nullptr;

        if (a_textureName.empty())
            return false;

        // Check if the texture has already been registered.
        // If so the associated image could be retrieved in the map.

        TextureNameImageNameMap::iterator textureImageIt = a_ioTextureNameImageNameMap.find(a_textureName);
        if (textureImageIt != a_ioTextureNameImageNameMap.end()) {
            std::string& imageName = textureImageIt->second;
            ImageNameToLuminateMap::iterator imageIt = a_ioImageToLuminateMap.find(imageName);
            a_outRedImage = imageIt->second;
            return true;
        }

        // Get texture definition

        ImageData imageData;
        bool success = getSegmentTextureImageData(a_segmentKeyPath, a_textureName, imageData);

        if (success) {
            // Create associated Luminate image and register it if not already registered.
            // As in 3DF/HPS image names are unique, we use names as index.

            RED::Object* redImage = nullptr;

            ImageNameToLuminateMap::iterator it = a_ioImageToLuminateMap.find(imageData.name);
            if (it == a_ioImageToLuminateMap.end()) {
                redImage = createREDIImage2D(imageData, a_resourceManager);
                a_ioImageToLuminateMap[imageData.name] = redImage;
            }
            else {
                redImage = it->second;
            }

            a_ioTextureNameImageNameMap[a_textureName] = imageData.name;
            a_outRedImage = redImage;
        }

        return success;
    }

    bool convertFromPBR(std::vector<HC_KEY> const& a_segmentKeyPath,
                        RED::Object* a_resourceManager,
                        ImageNameToLuminateMap& a_ioImageNameToLuminateMap,
                        TextureNameImageNameMap& a_ioTextureNameImageNameMap,
                        PBRToRealisticConversionMap& a_ioPBRToRealisticConversionMap,
                        RealisticMaterialInfo& a_ioMaterialInfo)
    {
        //////////////////////////////////////////
        // Try to retrieve PRB data from 3DF
        //////////////////////////////////////////

        float base_color_factor[4];
        char albedoTextureName[1024], normalTextureName[1024], emissiveTextureName[1024], metalnessTextureName[1024],
            occlusionTextureName[1024], roughnessTextureName[1024];
        int metalness_map_channel, roughness_map_channel, occlusion_map_channel;
        float normal_factor, metalness_factor, roughness_factor, occlusion_factor, alpha_factor;
        char options[1024] = "";

        bool hasPBR = HC_PShow_Net_PBR_Material((int)a_segmentKeyPath.size(),
                                                &a_segmentKeyPath[0],
                                                &albedoTextureName[0],
                                                &normalTextureName[0],
                                                &emissiveTextureName[0],
                                                &metalnessTextureName[0],
                                                &metalness_map_channel,
                                                &roughnessTextureName[0],
                                                &roughness_map_channel,
                                                &occlusionTextureName[0],
                                                &occlusion_map_channel,
                                                &base_color_factor,
                                                &normal_factor,
                                                &metalness_factor,
                                                &roughness_factor,
                                                &occlusion_factor,
                                                &alpha_factor,
                                                &options[0]);

        if (!hasPBR)
            return false;

        //////////////////////////////////////////
        // Check if this PBR material has already been converted.
        // If conversion result is already cached, we can use the already
        // converted textures, otherwise we have to convert them.
        //////////////////////////////////////////

        PBRToRealisticMaterialInput pbrToRealisticMaterialInput = createPBRToRealisticMaterialInput(
            RED::Color(base_color_factor[0], base_color_factor[1], base_color_factor[2], base_color_factor[3]),
            albedoTextureName,
            roughnessTextureName,
            metalnessTextureName,
            occlusionTextureName,
            normalTextureName,
            roughness_factor,
            metalness_factor,
            occlusion_factor,
            alpha_factor);

        if (!retrievePBRToRealisticMaterialConversion(
                pbrToRealisticMaterialInput, a_ioPBRToRealisticConversionMap, a_ioMaterialInfo)) {
            {
                // Retrieve or register RED images associated to texture names and
                // create associated RED images.

                RED::Object *albedoImage, *roughnessImage, *metalnessImage, *ambientOcclusionImage, *normalMapImage;

                registerTexture(a_segmentKeyPath,
                                pbrToRealisticMaterialInput.textureNameByChannel[PBRTextureChannels::Diffuse],
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                albedoImage);

                registerTexture(a_segmentKeyPath,
                                pbrToRealisticMaterialInput.textureNameByChannel[PBRTextureChannels::Roughness],
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                roughnessImage);

                registerTexture(a_segmentKeyPath,
                                pbrToRealisticMaterialInput.textureNameByChannel[PBRTextureChannels::Metalness],
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                metalnessImage);

                registerTexture(a_segmentKeyPath,
                                pbrToRealisticMaterialInput.textureNameByChannel[PBRTextureChannels::AmbientOcclusion],
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                ambientOcclusionImage);

                registerTexture(a_segmentKeyPath,
                                pbrToRealisticMaterialInput.textureNameByChannel[PBRTextureChannels::Normal],
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                normalMapImage);

                HC_KEY leafSegmentKey = a_segmentKeyPath.front();
                std::string leafSegmentKeyToString = std::to_string(leafSegmentKey);

                convertPBRToRealisticMaterial(a_resourceManager,
                                              leafSegmentKeyToString,
                                              albedoImage,
                                              roughnessImage,
                                              metalnessImage,
                                              ambientOcclusionImage,
                                              normalTextureName,
                                              pbrToRealisticMaterialInput,
                                              a_ioImageNameToLuminateMap,
                                              a_ioTextureNameImageNameMap,
                                              a_ioPBRToRealisticConversionMap,
                                              a_ioMaterialInfo);
            }
        }

        a_ioMaterialInfo.colorsByChannel[ColorChannels::DiffuseColor] = pbrToRealisticMaterialInput.diffuseColor;

        return hasPBR;
    }

    bool getFacesColor(std::vector<HC_KEY> const& a_segmentKeyPath,
                       std::string const& a_channel,
                       RED::Color& a_oColor,
                       std::string& a_oTextureName)
    {
        bool hasColor = false;

        float color[3] = {0.5, 0.5, 0.5};
        char texture[1024] = "";
        char options[1024] = "";

        if (HC_PShow_Net_Explicit_Color(
                (int)a_segmentKeyPath.size(), &a_segmentKeyPath[0], "faces", a_channel.c_str(), color, texture, options)) {
            if (streq(options, "ignore color")) {
                // TODO: Nothing to do ?
            }
            else if (streq(options, "FIndex")) {
                // TODO: FIndex colors are not implemented yet
            }
            else {
                if (a_channel == CHANNEL_DIFFUSE_COLOR || a_channel == CHANNEL_TRANSMISSION_COLOR) {
                    a_oColor = RED::Color(color[0], color[1], color[2]);
                    hasColor = true;
                }
            }

            // If texture name field is empty, then there is no texture.
            if ((a_channel == CHANNEL_DIFFUSE_TEXTURE || a_channel == CHANNEL_BUMP_TEXTURE || a_channel == CHANNEL_ENV_TEXTURE) &&
                strlen(texture) > 0) {
                a_oTextureName = texture;
                hasColor = true;
            }
        }

        return hasColor;
    }

    bool getFacesTexture(std::vector<HC_KEY> const& a_segmentKeyPath,
                         std::string const& a_channelName,
                         TextureChannelInfo& a_ioTextureChannelInfo)
    {
        RED::Color color;
        bool hasTexture = getFacesColor(a_segmentKeyPath, a_channelName, color, a_ioTextureChannelInfo.textureName);

        if (hasTexture) {
            // retrieve texture style
            char output[1024];
            HC_Show_One_Net_Texture(a_ioTextureChannelInfo.textureName.c_str(), "parameterization source", output);
            if (streq(output, "uv") || streq(output, "")) {
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::UV;
                a_ioTextureChannelInfo.textureMatrix = RED::Matrix::IDENTITY;
                float textureMatrix[16];
                if (HC_PShow_Net_Texture_Matrix((int)a_segmentKeyPath.size(), &a_segmentKeyPath[0], textureMatrix)) {
                    a_ioTextureChannelInfo.textureMatrix = RED::Matrix(textureMatrix);
                }
            }
            else if (streq(output, "cylinder"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Cylinder;
            else if (streq(output, "natural uv"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::NaturalUV;
            else if (streq(output, "object"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Object;
            else if (streq(output, "physical reflection"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::PhysicalReflection;
            else if (streq(output, "reflection vector"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::ReflectionVector;
            else if (streq(output, "surface normal"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::SurfaceNormal;
            else if (streq(output, "sphere"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Sphere;
            else if (streq(output, "world"))
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::World;
        }

        return hasTexture;
    }

    RealisticMaterialInfo getSegmentMaterialInfo(std::vector<HC_KEY> const& a_segmentKeyPath,
                                                 RED::Object* a_resourceManager,
                                                 RealisticMaterialInfo const& a_baseMaterialInfo,
                                                 ImageNameToLuminateMap& a_ioImageNameToLuminateMap,
                                                 TextureNameImageNameMap& a_ioTextureNameImageNameMap,
                                                 PBRToRealisticConversionMap& a_ioPBRToRealisticConversionMap)
    {
        RealisticMaterialInfo materialInfo = a_baseMaterialInfo;
        RED::Object* redImage;

        if (!convertFromPBR(a_segmentKeyPath,
                            a_resourceManager,
                            a_ioImageNameToLuminateMap,
                            a_ioTextureNameImageNameMap,
                            a_ioPBRToRealisticConversionMap,
                            materialInfo)) {
            std::string textureName;

            // Determine diffuse color.
            getFacesColor(
                a_segmentKeyPath, CHANNEL_DIFFUSE_COLOR, materialInfo.colorsByChannel[ColorChannels::DiffuseColor], textureName);

            // Determine transmission color.
            // If no transmission color, default = black color.
            getFacesColor(a_segmentKeyPath,
                          CHANNEL_TRANSMISSION_COLOR,
                          materialInfo.colorsByChannel[ColorChannels::TransmissionColor],
                          textureName);

            // Retrieve the segment diffuse texture name if it exists
            if (getFacesTexture(a_segmentKeyPath,
                                CHANNEL_DIFFUSE_TEXTURE,
                                materialInfo.textureNameByChannel[TextureChannels::DiffuseTexture])) {
                registerTexture(a_segmentKeyPath,
                                materialInfo.textureNameByChannel[TextureChannels::DiffuseTexture].textureName,
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                redImage);
            }

            // Determine bump map
            if (getFacesTexture(
                    a_segmentKeyPath, CHANNEL_BUMP_TEXTURE, materialInfo.textureNameByChannel[TextureChannels::BumpTexture])) {
                registerTexture(a_segmentKeyPath,
                                materialInfo.textureNameByChannel[TextureChannels::BumpTexture].textureName,
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                redImage);
            }
        }

        return materialInfo;
    }

    RED_RC computeUVParameterization(const RED::MESH_CHANNEL& channel,
                                     UVParameterization const& uvParam,
                                     HC_KEY a_key,
                                     const RED::State& a_state,
                                     int a_point_count,
                                     RED::IMeshShape* a_imesh)
    {
        float* uvs = nullptr;
        float* fPoints = nullptr;

        switch (uvParam) {
            case UVParameterization::World:

                uvs = new float[a_point_count * 2];

                RC_CHECK(a_imesh->GetArray((void const*&)fPoints, RED::MCL_VERTEX)); // points should be in world coordinates

                for (int i = 0; i < a_point_count; i++) {
                    uvs[2 * i] = *(fPoints + 3 * i);
                    uvs[2 * i + 1] = *(fPoints + 3 * i + 1);
                }

                RC_CHECK(a_imesh->SetArray(channel, uvs, a_point_count, 2, RED::MFT_FLOAT, a_state));
                delete[] uvs;

                break;
            case UVParameterization::Cylinder:
                RC_CHECK(a_imesh->BuildTextureCoordinates(channel, RED::MTCM_CYLINDRICAL_CAP, RED::Matrix::IDENTITY, a_state));
                break;
            case UVParameterization::Sphere:
                RC_CHECK(a_imesh->BuildTextureCoordinates(channel, RED::MTCM_SPHERICAL, RED::Matrix::IDENTITY, a_state));
                break;
            default:

                int uvDimensions = 0;
                HC_MShow_Vertex_Parameter_Size(a_key, &uvDimensions);
                if (uvDimensions > 0) {
                    if (uvDimensions == 2 || uvDimensions == 3) {
                        uvs = new float[a_point_count * uvDimensions];
                        HC_MShow_Vertex_Parameters(a_key, 0, a_point_count, &uvDimensions, uvs);

                        // There is a problem with UV coordinate between 3DF -> Luminate
                        // One of them is lying
                        for (int i = 0; i < (a_point_count * uvDimensions); i += uvDimensions) {
                            uvs[i + 1] = 1.f - uvs[i + 1]; // Y
                        }

                        RC_CHECK(a_imesh->SetArray(RED::MCL_TEX0, uvs, a_point_count, uvDimensions, RED::MFT_FLOAT, a_state));
                        delete[] uvs;
                    }
                }
                else // Luminate need UV coordinates so we will build them
                {
                    RC_CHECK(a_imesh->BuildTextureCoordinates(RED::MCL_TEX0, RED::MTCM_BOX, RED::Matrix::IDENTITY, a_state));
                }

                break;
        }

        return RED_OK;
    }

    RED_RC convertVertexData(HC_KEY a_key,
                             const RED::State& a_state,
                             int a_point_count,
                             RED::IMeshShape* a_imesh,
                             float* a_normals,
                             RealisticMaterialInfo const& a_material)
    {
        // Normals are needed if UV creation is called
        RC_TEST(setVertexNormals(a_state, a_imesh, a_point_count, a_normals));

        bool hasSomeTexture = std::any_of(a_material.textureNameByChannel.begin(),
                                          a_material.textureNameByChannel.end(),
                                          [](TextureChannelInfo const& a_textureChannelInfo) {
                                              return !a_textureChannelInfo.textureName.empty();
                                          });

        if (hasSomeTexture) {
            UVParameterization diffuseUVParam =
                a_material.textureNameByChannel[TextureChannels::DiffuseTexture].textureParameterization;
            computeUVParameterization(RED::MESH_CHANNEL::MCL_TEX0, diffuseUVParam, a_key, a_state, a_point_count, a_imesh);

            UVParameterization bumpUVParam =
                a_material.textureNameByChannel[TextureChannels::BumpTexture].textureParameterization;
            if (bumpUVParam != diffuseUVParam) {
                computeUVParameterization(RED::MESH_CHANNEL::MCL_TEX1, bumpUVParam, a_key, a_state, a_point_count, a_imesh);
            }
        }

        // Create the vertex information used for shading tangent-space generation.
        // The channels mentionned here must match the SetupRealisticMaterial bump channels.
        // We reserve MCL_TEX0 for 3DF texture coordinates (if any, so use another channel)
        RC_CHECK(a_imesh->BuildTextureCoordinates(RED::MCL_TEX7, RED::MTCM_BOX, RED::Matrix::IDENTITY, a_state));
        RC_CHECK(a_imesh->BuildTangents(RED::MCL_USER0, RED::MCL_TEX7, a_state));

        return RED_OK;
    }

    RED::Object* convertREDMeshShapeFromPolygon(HC_KEY a_key, RealisticMaterialInfo const& a_material, const RED::State& a_state)
    {
        int point_count = 0;
        RED::Object* shape = nullptr;

        HC_Show_Polygon_Count(a_key, &point_count);

        if (point_count > 0) {
            HIC_Point* points = new HIC_Point[point_count];
            HC_Show_Polygon(a_key, nullptr, points);

            shape = createPolygonREDMeshShape(a_state, point_count, (float*)points);

            if (shape) {
                float* normals = new float[3 * point_count];
                HC_MShow_Net_Vertex_Normals(a_key, 0, point_count, normals);
                convertVertexData(a_key, a_state, point_count, shape->As<RED::IMeshShape>(), normals, a_material);
                delete[] normals;
            }

            delete[] points;
        }

        return shape;
    }

    RED::Object*
        convertREDMeshShapeFromShellTriStrips(HC_KEY a_key, RealisticMaterialInfo const& a_material, const RED::State& a_state)
    {
        int point_count = 0;
        int tristrips_length = 0;
        int face_indices_length = 0;
        RED::Object* shape = nullptr;

        HC_Show_Shell_By_Tristrips_Size(a_key, &point_count, &tristrips_length, &face_indices_length);

        if (point_count > 0 && tristrips_length > 0 && face_indices_length > 0) {
            HIC_Point* points = new HIC_Point[point_count];
            int* tristrips = new int[tristrips_length];
            int* face_indices = new int[face_indices_length];
            HC_Show_Shell_By_Tristrips(
                a_key, &point_count, points, &tristrips_length, tristrips, &face_indices_length, face_indices);

            float* normals = new float[3 * point_count];
            HC_MShow_Net_Vertex_Normals(a_key, 0, point_count, normals);

            shape = createREDMeshShapeFromTriStrips(a_state, tristrips_length, tristrips, point_count, (float*)points, normals);

            if (shape) {
                convertVertexData(a_key, a_state, point_count, shape->As<RED::IMeshShape>(), normals, a_material);
            }

            delete[] normals;
            delete[] tristrips;
            delete[] face_indices;
            delete[] points;
        }

        return shape;
    }

    std::string serializeKeyPath(std::vector<HC_KEY> const& a_ioKeyPath)
    {
        std::string serialized = "";
        bool first = true;

        for (std::vector<HC_KEY>::const_iterator it = a_ioKeyPath.cbegin(); it != a_ioKeyPath.cend(); ++it) {
            const HC_KEY& key = *it;

            if (first)
                first = false;
            else
                serialized += ",";

            serialized += std::to_string(key).c_str();
        }
        return serialized;
    }

    RED::Object* convertSegmentTree(RED::Object* a_resourceManager,
                                    ConversionContext3DF& a_ioConversionContext,
                                    HC_KEY a_segmentKey,
                                    std::vector<HC_KEY>& a_ioKeyPath)
    {
        RED::IResourceManager* iresourceManager = a_resourceManager->As<RED::IResourceManager>();

        //////////////////////////////////////////
        // Skip invisible segment conversion
        //////////////////////////////////////////

        if (!isSegmentVisible(a_ioKeyPath))
            return nullptr;

        //////////////////////////////////////////
        // Apply segment transform matrix it has one.
        // We want here the individual matrix, not the net one
        // because we benifit of Luminate transform stack as well.
        //////////////////////////////////////////

        RED::Matrix redMatrix = RED::Matrix::IDENTITY;

        if (HC_Show_Existence("modelling matrix")) {
            float vizMatrix[16];
            HC_Show_Modelling_Matrix(vizMatrix);
            redMatrix.SetColumnMajorMatrix(vizMatrix);
        }

        //////////////////////////////////////////
        // Search and convert supported segment geometry
        //////////////////////////////////////////

        std::string searchFilter = "cylinder,mesh,polycylinder,shell,sphere,polygon";
        bool hasGeometry = HC_Show_Existence(searchFilter.c_str()) > 0;
        RED::Object* material = nullptr;
        std::vector<RED::Object*> meshShapes;

        // serialize keyPath
        std::string serializedKeyPath = serializeKeyPath(a_ioKeyPath);

        if (hasGeometry) {
            // Create a RED material only for segment with geometries and we build it along its full key path to manage correctly
            // attributes and includes.

            RealisticMaterialInfo materialInfo = getSegmentMaterialInfo(a_ioKeyPath,
                                                                        a_resourceManager,
                                                                        a_ioConversionContext.defaultMaterialInfo,
                                                                        a_ioConversionContext.imageNameToLuminateMap,
                                                                        a_ioConversionContext.textureNameImageNameMap,
                                                                        a_ioConversionContext.pbrToRealisticConversionMap);

            material = createREDMaterial(materialInfo,
                                         a_resourceManager,
                                         a_ioConversionContext.imageNameToLuminateMap,
                                         a_ioConversionContext.textureNameImageNameMap,
                                         a_ioConversionContext.materials);

            // Check if the geometry of the current segment has already be converted, thus it can be shared.

            SegmentMeshShapesMap::iterator segmentMeshShapesIt = a_ioConversionContext.segmentMeshShapesMap.find(a_segmentKey);

            if (segmentMeshShapesIt != a_ioConversionContext.segmentMeshShapesMap.end()) {
                meshShapes = segmentMeshShapesIt->second;
            }
            else {
                // Convert the each geometry.

                HC_Begin_Contents_Search(".", searchFilter.c_str());

                HC_KEY key;
                char type[64];

                while (HC_Find_Contents(type, &key)) {
                    RED::Object* shape;
                    if (!strcmp(type, "polygon")) {
                        shape = convertREDMeshShapeFromPolygon(key, materialInfo, iresourceManager->GetState());
                    }
                    else {
#if DEBUG_DISABLE_SHELL_TRISTRIPS
                        shape = convertREDMeshShapeFromShell(key, materialInfo, state);
#else
                        shape = convertREDMeshShapeFromShellTriStrips(key, materialInfo, iresourceManager->GetState());
#endif
                    }
                    if (shape) {
                        meshShapes.push_back(shape);
                    }
                }
                HC_End_Contents_Search();

                a_ioConversionContext.segmentMeshShapesMap[a_segmentKey] = meshShapes;
            }
        }

        //////////////////////////////////////////
        // Create RED transform shape associated to segment
        //////////////////////////////////////////

        RED::Object* transform = RED::Factory::CreateInstance(CID_REDTransformShape);
        RED::ITransformShape* itransform = transform->As<RED::ITransformShape>();

        // Register transform shape associated to the segment.
        a_ioConversionContext.segmentTransformShapeMap[serializedKeyPath] = transform;

        // Apply transform matrix.
        RC_CHECK(itransform->SetMatrix(&redMatrix, iresourceManager->GetState()));

        // Apply material if any.
        if (material != nullptr)
            RC_CHECK(transform->As<RED::IShape>()->SetMaterial(material, iresourceManager->GetState()));

        // Add geometry shapes if any.
        for (RED::Object* meshShape: meshShapes)
            RC_CHECK(itransform->AddChild(meshShape, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState()));

        //////////////////////////////////////////
        // Process segment children
        //////////////////////////////////////////

        HC_Begin_Contents_Search(".", "segments");
        {
            HC_KEY key;
            char type[64];
            while (HC_Find_Contents(type, &key)) {
                HC_Open_Segment_By_Key(key);

                a_ioKeyPath.insert(a_ioKeyPath.begin(), key);
                RED::Object* child = convertSegmentTree(a_resourceManager, a_ioConversionContext, key, a_ioKeyPath);
                a_ioKeyPath.erase(a_ioKeyPath.begin());

                if (child)
                    itransform->AddChild(child, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());
                HC_Close_Segment();
            }
        }
        HC_End_Contents_Search();

        //////////////////////////////////////////
        // Process segment includes.
        // Due to attributes propagation in 3DF, we cannot
        // share the full include subgraph but we have to duplicate
        // it in order to apply different materials in Luminate.
        // Thus a segment include acts like a child but we
        // store the include key in the keypath.
        //////////////////////////////////////////

        HC_Begin_Contents_Search(".", "include");
        {
            HC_KEY key;
            char type[64];
            while (HC_Find_Contents(type, &key)) {
                HC_KEY ikey = HC_Show_Include_Segment(key, nullptr);

                HC_Open_Segment_By_Key(ikey);

                a_ioKeyPath.insert(a_ioKeyPath.begin(), key);
                a_ioKeyPath.insert(a_ioKeyPath.begin(), ikey);
                RED::Object* included = convertSegmentTree(a_resourceManager, a_ioConversionContext, ikey, a_ioKeyPath);
                a_ioKeyPath.erase(a_ioKeyPath.begin());
                a_ioKeyPath.erase(a_ioKeyPath.begin());

                HC_Close_Segment();

                if (included)
                    itransform->AddChild(included, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());
            }
        }
        HC_End_Contents_Search();

        return transform;
    }

    LuminateSceneInfoPtr convert3DFSceneToLuminate(HBaseView* a_3dfView)
    {
        //////////////////////////////////////////
        // Get the resource manager singleton.
        //////////////////////////////////////////

        RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
        RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();

        //////////////////////////////////////////
        // Create conversion root shape
        //////////////////////////////////////////

        RED::Object* rootTransformShape = RED::Factory::CreateInstance(CID_REDTransformShape);
        RED::ITransformShape* itransform = rootTransformShape->As<RED::ITransformShape>();

        //////////////////////////////////////////
        // Manage scene handedness.
        // Luminate is working with a right-handed coordinate system,
        // thus if the input scene is left-handed we must apply
        // a transformation on the conversion root to go right-handed.
        //////////////////////////////////////////

        Handedness viewHandedness = getViewHandedness(a_3dfView);

        if (viewHandedness == Handedness::LeftHanded) {
            RED::Matrix LeftHandedToRightHandedMatrix(
                RED::Vector3(1, 0, 0), RED::Vector3(0, 0, 1), RED::Vector3(0, 1, 0), RED::Vector3(0, 0, 0));
            RC_CHECK(itransform->SetMatrix(&LeftHandedToRightHandedMatrix, iresourceManager->GetState()));
        }

        //////////////////////////////////////////
        // Create the default material definition.
        //////////////////////////////////////////

        RealisticMaterialInfo defaultMaterialInfo = createDefaultRealisticMaterialInfo();

        //////////////////////////////////////////
        // Initialize the conversion context which will hold
        // current conversion data state during conversion.
        //////////////////////////////////////////

        ConversionContext3DFPtr sceneInfoPtr = std::make_shared<ConversionContext3DF>();
        sceneInfoPtr->rootTransformShape = rootTransformShape;
        sceneInfoPtr->defaultMaterialInfo = defaultMaterialInfo;
        sceneInfoPtr->viewHandedness = viewHandedness;

        //////////////////////////////////////////
        // Proceed with segment tree traversal to convert scene
        //////////////////////////////////////////

        HC_Open_Segment_By_Key(a_3dfView->GetModelKey());

        std::vector<HC_KEY> segmentKeyPath = {a_3dfView->GetModelKey()};
        RED::Object* result = convertSegmentTree(resourceManager, *sceneInfoPtr, a_3dfView->GetModelKey(), segmentKeyPath);

        if (result)
            itransform->AddChild(result, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());

        HC_Close_Segment();

        return sceneInfoPtr;
    }
} // namespace hoops_luminate_bridge
