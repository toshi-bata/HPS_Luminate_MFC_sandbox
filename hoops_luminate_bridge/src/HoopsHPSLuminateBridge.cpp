#include <sstream>
#include <algorithm>
#include <map>
#include <vector>

#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>
#include <hoops_luminate_bridge/LuminateRCTest.h>

#include <REDObject.h>
#include <REDVector3.h>
#include <REDIResourceManager.h>
#include <REDIImage2D.h>
#include <REDFactory.h>
#include <REDITransformShape.h>
#include <REDIShape.h>

namespace hoops_luminate_bridge {

    HoopsLuminateBridgeHPS::HoopsLuminateBridgeHPS(HPS::View* a_hpsView): m_hpsView(a_hpsView) {}

    HoopsLuminateBridgeHPS::~HoopsLuminateBridgeHPS() {}

    void HoopsLuminateBridgeHPS::saveCameraState() { m_hpsView->GetSegmentKey().ShowCamera(m_hpsCamera); }

    LuminateSceneInfoPtr HoopsLuminateBridgeHPS::convertScene() { return convertHPSSceneToLuminate(m_hpsView); }

    bool HoopsLuminateBridgeHPS::checkCameraChange()
    {
        HPS::CameraKit camera;
        m_hpsView->GetSegmentKey().ShowCamera(camera);

        bool hasChanged = m_hpsCamera != camera;

        if (hasChanged)
            m_hpsCamera = camera;

        return hasChanged;
    }

    CameraInfo HoopsLuminateBridgeHPS::getCameraInfo() { return getHPSCameraInfo(m_hpsView); }

    CameraInfo getHPSCameraInfo(HPS::View* a_hpsView)
    {
        //////////////////////////////////////////
        // Get the HPS camera.
        //////////////////////////////////////////

        HPS::CameraKit camera;
        a_hpsView->GetSegmentKey().ShowCamera(camera);

        //////////////////////////////////////////
        // Compute camera axis in world space.
        //////////////////////////////////////////

        HPS::Point hpsTarget, hpsPosition;
        HPS::Vector hpsUpVector;
        camera.ShowTarget(hpsTarget);
        camera.ShowUpVector(hpsUpVector);
        camera.ShowPosition(hpsPosition);

        RED::Vector3 eyePosition, right, top, sight, target;

        target = RED::Vector3(hpsTarget.x, hpsTarget.y, hpsTarget.z);
        top = RED::Vector3(hpsUpVector.x, hpsUpVector.y, hpsUpVector.z);
        eyePosition = RED::Vector3(hpsPosition.x, hpsPosition.y, hpsPosition.z);

        sight = target - eyePosition;
        sight.Normalize();

        right = top.Cross2(sight);
        right.Normalize();

        //////////////////////////////////////////
        // Get camera projection mode
        //////////////////////////////////////////

        HPS::Camera::Projection hpsProjectionMode;
        camera.ShowProjection(hpsProjectionMode);

        if (hpsProjectionMode == HPS::Camera::Projection::Default)
            HPS::CameraKit::GetDefault().ShowProjection(hpsProjectionMode);

        ProjectionMode projectionMode = ProjectionMode::Perspective;
        if (hpsProjectionMode == HPS::Camera::Projection::Perspective)
            projectionMode = ProjectionMode::Perspective;
        else if (hpsProjectionMode == HPS::Camera::Projection::Orthographic)
            projectionMode = ProjectionMode::Orthographic;
        else if (hpsProjectionMode == HPS::Camera::Projection::Stretched)
            projectionMode = ProjectionMode::Stretched;

        //////////////////////////////////////////
        // Get camera field
        //////////////////////////////////////////

        float hpsFieldWidth, hpsFieldHeight;
        camera.ShowField(hpsFieldWidth, hpsFieldHeight);

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
        cameraInfo.field_width = hpsFieldWidth;
        cameraInfo.field_height = hpsFieldHeight;

        return cameraInfo;
    }

    Handedness getViewHandedness(HPS::View* a_hpsView)
    {
        HPS::DrawingAttributeKit drawingAttributeKit;
        Handedness handedness = Handedness::RightHanded; // Default is RightHanded in HPS

        if (a_hpsView->GetSegmentKey().ShowDrawingAttribute(drawingAttributeKit)) {
            HPS::Drawing::Handedness hpsHandedness;
            if (drawingAttributeKit.ShowWorldHandedness(hpsHandedness)) {
                switch (hpsHandedness) {
                    case HPS::Drawing::Handedness::Left:
                        handedness = Handedness::LeftHanded;
                        break;
                    case HPS::Drawing::Handedness::Right:
                        handedness = Handedness::RightHanded;
                        break;
                    default:; // None handedness, use default
                }
            }
        }

        return handedness;
    }

    bool isSegmentVisible(const HPS::KeyPath& a_segmentKeyPath)
    {
        bool facesAreVisible = true;

        HPS::VisibilityKit visibilityKit;

        // Retrieve visibility informations.
        // If the keypath has no visibility data, then it is considered
        // as visible.

        if (a_segmentKeyPath.ShowNetVisibility(visibilityKit)) {
            visibilityKit.ShowFaces(facesAreVisible);
        }

        return facesAreVisible;
    }

    ImageData getImageData(HPS::ImageKit a_imageKit)
    {
        unsigned int width, height;
        a_imageKit.ShowSize(width, height);

        HPS::Image::Format format;
        a_imageKit.ShowFormat(format);

        // Determine bytes per pixel according to image format.
        // The conversion supports for now only RGBA and RGB formats.

        int bytesPerPixel = 0;

        switch (format) {
            case HPS::Image::Format::RGB:
                bytesPerPixel = 3;
                break;
            case HPS::Image::Format::RGBA:
                bytesPerPixel = 4;
                break;
            default:
                bytesPerPixel = 4;
                break;
        }

        HPS::ByteArray hpsPixelData;
        a_imageKit.ShowData(hpsPixelData);

        unsigned char* pixelData = new unsigned char[width * height * bytesPerPixel];
        memcpy(pixelData, hpsPixelData.data(), width * height * bytesPerPixel);

        // Register the image data

        ImageData imageData;
        imageData.width = width;
        imageData.height = height;
        imageData.bytesPerPixel = bytesPerPixel;
        imageData.pixelData = pixelData;

        return imageData;
    }

    bool getSegmentTextureImageData(const HPS::KeyPath& a_segmentKeyPath,
                                    std::string const& a_textureName,
                                    ImageData& a_outImageData)
    {
        bool success = false;

        HPS::TextureDefinition textureDefinition;
        if (a_segmentKeyPath.ShowEffectiveTextureDefinition(a_textureName.c_str(), textureDefinition)) {
            HPS::ImageDefinition imageDefinition;

            // Retrieve the image definition from the same portfolio.
            // This can fail if the image is not in the same portfolio, in this
            // case, retrieve it from a different portfolio.
            if (!textureDefinition.ShowSource(imageDefinition)) {
                HPS::UTF8 hpsImageName;
                if (textureDefinition.ShowSource(hpsImageName)) {
                    a_segmentKeyPath.ShowEffectiveImageDefinition(hpsImageName, imageDefinition);
                }
            }

            if (!imageDefinition.Empty()) {
                HPS::ImageKit imageKit;
                imageDefinition.Show(imageKit);

                // Lazy support of all formats, but not the most optimal...
                imageKit.Convert(HPS::Image::Format::RGBA);

                a_outImageData = getImageData(imageKit);
                a_outImageData.name = imageDefinition.Name();
                success = true;
            }
        }

        return success;
    }

    RED::Object* getSegmentTextureRedImage(const HPS::KeyPath& a_segmentKeyPath,
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

    bool registerTexture(const HPS::KeyPath& a_segmentKeyPath,
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

    bool convertFromPBR(const HPS::KeyPath& a_segmentKeyPath,
                        RED::Object* a_resourceManager,
                        ImageNameToLuminateMap& a_ioImageNameToLuminateMap,
                        TextureNameImageNameMap& a_ioTextureNameImageNameMap,
                        PBRToRealisticConversionMap& a_ioPBRToRealisticConversionMap,
                        RealisticMaterialInfo& a_ioMaterialInfo)
    {
        //////////////////////////////////////////
        // Try to retrieve PRB data
        //////////////////////////////////////////

        HPS::PBRMaterialKit pbrMaterialKit;
        bool hasPBR = a_segmentKeyPath.ShowNetPBRMaterial(pbrMaterialKit);

        if (!hasPBR)
            return false;

        //////////////////////////////////////////
        // Check if this PBR material has already been converted.
        // If conversion result is already cached, we can use the already
        // converted textures, otherwise we have to convert them.
        //////////////////////////////////////////

        HPS::RGBAColor baseColorFactor = HPS::RGBAColor(0.0, 0.0, 0.0);
        pbrMaterialKit.ShowBaseColorFactor(baseColorFactor);

        HPS::UTF8 albedoTextureName = "";
        pbrMaterialKit.ShowBaseColorMap(albedoTextureName);

        HPS::UTF8 roughnessTextureName = "";
        HPS::Material::Texture::ChannelMapping roughnessChannelMapping;
        float roughnessFactor = 1.0f;
        pbrMaterialKit.ShowRoughnessMap(roughnessTextureName, roughnessChannelMapping);
        pbrMaterialKit.ShowRoughnessFactor(roughnessFactor);

        HPS::UTF8 metalnessTextureName = "";
        HPS::Material::Texture::ChannelMapping metalnessChannelMapping;
        float metalnessFactor = 1.0f;
        pbrMaterialKit.ShowMetalnessMap(metalnessTextureName, metalnessChannelMapping);
        pbrMaterialKit.ShowMetalnessFactor(metalnessFactor);

        HPS::UTF8 occlusionTextureName = "";
        HPS::Material::Texture::ChannelMapping occlusionChannelMapping;
        float occlusionFactor = 1.0f;
        pbrMaterialKit.ShowOcclusionMap(occlusionTextureName, occlusionChannelMapping);
        pbrMaterialKit.ShowOcclusionFactor(occlusionFactor);

        HPS::UTF8 normalTextureName = "";
        pbrMaterialKit.ShowNormalMap(normalTextureName);

        float alphaFactor = 1.0f;
        bool alphaMask = false;
        pbrMaterialKit.ShowAlphaFactor(alphaFactor, alphaMask);

        PBRToRealisticMaterialInput pbrToRealisticMaterialInput = createPBRToRealisticMaterialInput(
            RED::Color(baseColorFactor.red, baseColorFactor.green, baseColorFactor.blue, baseColorFactor.alpha),
            albedoTextureName.GetBytes(),
            roughnessTextureName.GetBytes(),
            metalnessTextureName.GetBytes(),
            occlusionTextureName.GetBytes(),
            normalTextureName.GetBytes(),
            roughnessFactor,
            metalnessFactor,
            occlusionFactor,
            alphaFactor);

        if (!retrievePBRToRealisticMaterialConversion(
                pbrToRealisticMaterialInput, a_ioPBRToRealisticConversionMap, a_ioMaterialInfo)) {
            // Get a unique name for the leaf segment in order to set unique names for converted textures.

            HPS::Key leafKey = a_segmentKeyPath.Back();
            std::string leafKeyToString = std::to_string(leafKey.GetHash());

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

            convertPBRToRealisticMaterial(a_resourceManager,
                                          leafKeyToString,
                                          albedoImage,
                                          roughnessImage,
                                          metalnessImage,
                                          ambientOcclusionImage,
                                          normalTextureName.GetBytes(),
                                          pbrToRealisticMaterialInput,
                                          a_ioImageNameToLuminateMap,
                                          a_ioTextureNameImageNameMap,
                                          a_ioPBRToRealisticConversionMap,
                                          a_ioMaterialInfo);
        }

        a_ioMaterialInfo.colorsByChannel[ColorChannels::DiffuseColor] = pbrToRealisticMaterialInput.diffuseColor;

        return hasPBR;
    }

    bool HoopsLuminateBridgeHPS::getFacesColor(const HPS::KeyPath& a_segmentKeyPath,
                                               HPS::Material::Channel a_channel,
                                               RED::Color& a_oColor,
                                               std::string& a_oTextureName)
    {
        bool hasColor = false;
        std::string texture = "";

        HPS::MaterialMappingKit materialMappingKit;
        if (a_segmentKeyPath.ShowNetMaterialMapping(materialMappingKit)) {
            HPS::Material::Type faceMaterialType;
            HPS::MaterialKit faceMaterialKit;
            float value;

            HPS::RGBAColor color;
            HPS::UTF8 texname;
            if (materialMappingKit.ShowFaceChannel(a_channel, faceMaterialType, color, texname, value)) {
                a_oColor = RED::Color(color.red, color.green, color.blue, color.alpha);
                a_oTextureName = texname;
                hasColor = true;
            }
        }

        return hasColor;
    }

    void HoopsLuminateBridgeHPS::updateSelectedSegmentTransform()
    {
        ConversionContextHPS* conversionDataHPS = (ConversionContextHPS*)m_conversionDataPtr.get();
        SelectedHPSSegmentInfoPtr selectedHPSSegment = std::dynamic_pointer_cast<SelectedHPSSegmentInfo>(m_selectedSegment);

        if (selectedHPSSegment == nullptr)
            return;

        hoops_luminate_bridge::SegmentTransformShapeMap::iterator it =
            conversionDataHPS->segmentTransformShapeMap.find(selectedHPSSegment->m_serializedKeyPath);

        if (it != conversionDataHPS->segmentTransformShapeMap.end()) {
            HPS::MatrixKit matrixKit;
            selectedHPSSegment->m_segmentKey.ShowModellingMatrix(matrixKit);

            float vizMatrix[16];
            matrixKit.ShowElements(vizMatrix);

            RED::Matrix redMatrix = RED::Matrix::IDENTITY;
            redMatrix.SetColumnMajorMatrix(vizMatrix);

            RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
            RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();
            RED::ITransformShape* itransform = it->second->As<RED::ITransformShape>();
            itransform->SetMatrix(&redMatrix, iresourceManager->GetState());

            resetFrame();
        }

        m_selectedSegmentTransformIsDirty = false;
    }

    std::vector<RED::Object*> HoopsLuminateBridgeHPS::getSelectedLuminateMeshShapes()
    {
        if (m_selectedSegment != nullptr) {
            ConversionContextHPS* conversionDataHPS = (ConversionContextHPS*)m_conversionDataPtr.get();
            SelectedHPSSegmentInfoPtr selectedHPSSegment = std::dynamic_pointer_cast<SelectedHPSSegmentInfo>(m_selectedSegment);

            hoops_luminate_bridge::SegmentMeshShapesMap::iterator it =
                conversionDataHPS->segmentMeshShapesMap.find(selectedHPSSegment->m_instanceID);

            if (it != conversionDataHPS->segmentMeshShapesMap.end()) {
                return it->second;
            }
        }
        return std::vector<RED::Object*>();
    }

    RED::Object* HoopsLuminateBridgeHPS::getSelectedLuminateTransformNode()
    {
        if (m_selectedSegment != nullptr) {
            ConversionContextHPS* conversionDataHPS = (ConversionContextHPS*)m_conversionDataPtr.get();
            SelectedHPSSegmentInfoPtr selectedHPSSegment = std::dynamic_pointer_cast<SelectedHPSSegmentInfo>(m_selectedSegment);

            hoops_luminate_bridge::SegmentTransformShapeMap::iterator it =
                conversionDataHPS->segmentTransformShapeMap.find(selectedHPSSegment->m_serializedKeyPath);

            if (it != conversionDataHPS->segmentTransformShapeMap.end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    RED_RC HoopsLuminateBridgeHPS::syncRootTransform()
    {
        HPS::SegmentKey modelSegmentKey = m_hpsView->GetAttachedModel().GetSegmentKey();
        HPS::MatrixKit modellingMatrix;
        if (modelSegmentKey.ShowModellingMatrix(modellingMatrix)) {
            // get current matrix
            float vizMatrix[16];
            modellingMatrix.ShowElements(vizMatrix);

            RED::Object* resourceManager = RED::Factory::CreateInstance(CID_REDResourceManager);
            RED::IResourceManager* iresourceManager = resourceManager->As<RED::IResourceManager>();
            LuminateSceneInfoPtr sceneInfo = m_conversionDataPtr;
            RED::ITransformShape* itransform = sceneInfo->rootTransformShape->As<RED::ITransformShape>();

            // Apply transform matrix.
            RED::Matrix redMatrix;
            redMatrix.SetColumnMajorMatrix(vizMatrix);

            // Restore left handedness transform.
            if (m_conversionDataPtr->viewHandedness == Handedness::LeftHanded) {
                redMatrix = HoopsLuminateBridge::s_leftHandedToRightHandedMatrix * redMatrix;
            }

            RC_CHECK(itransform->SetMatrix(&redMatrix, iresourceManager->GetState()));
            resetFrame();
        }

        m_rootTransformIsDirty = false;

        return RED_RC();
    }

    bool getFacesTexture(const HPS::KeyPath& a_segmentKeyPath,
                         HPS::Material::Channel a_channel,
                         TextureChannelInfo& a_ioTextureChannelInfo)
    {
        RED::Color color;
        bool hasTexture =
            HoopsLuminateBridgeHPS::getFacesColor(a_segmentKeyPath, a_channel, color, a_ioTextureChannelInfo.textureName);

        if (hasTexture) {
            // retrieve texture style
            HPS::TextureDefinition definition;
            HPS::TextureOptionsKit options;
            HPS::Material::Texture::Parameterization param;
            a_segmentKeyPath.ShowEffectiveTextureDefinition(a_ioTextureChannelInfo.textureName.c_str(), definition);
            definition.ShowOptions(options);
            options.ShowParameterizationSource(param);
            if (param == HPS::Material::Texture::Parameterization::UV) {
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::UV;
                a_ioTextureChannelInfo.textureMatrix = RED::Matrix::IDENTITY;

                HPS::MatrixKit matrixKit;
                if (a_segmentKeyPath.ShowNetTextureMatrix(matrixKit)) {
                    float matrix[16];
                    if (matrixKit.ShowElements(matrix))
                        a_ioTextureChannelInfo.textureMatrix = RED::Matrix(matrix);
                }
            }
            else if (param == HPS::Material::Texture::Parameterization::Cylinder)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Cylinder;
            else if (param == HPS::Material::Texture::Parameterization::NaturalUV)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::NaturalUV;
            else if (param == HPS::Material::Texture::Parameterization::Object)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Object;
            else if (param == HPS::Material::Texture::Parameterization::PhysicalReflection)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::PhysicalReflection;
            else if (param == HPS::Material::Texture::Parameterization::ReflectionVector)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::ReflectionVector;
            else if (param == HPS::Material::Texture::Parameterization::SurfaceNormal)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::SurfaceNormal;
            else if (param == HPS::Material::Texture::Parameterization::Sphere)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::Sphere;
            else if (param == HPS::Material::Texture::Parameterization::World)
                a_ioTextureChannelInfo.textureParameterization = UVParameterization::World;
        }

        return hasTexture;
    }

    RealisticMaterialInfo getSegmentMaterialInfo(const HPS::KeyPath& a_segmentKeyPath,
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
            HoopsLuminateBridgeHPS::getFacesColor(a_segmentKeyPath,
                                                  HPS::Material::Channel::DiffuseColor,
                                                  materialInfo.colorsByChannel[ColorChannels::DiffuseColor],
                                                  textureName);

            // Determine transmission color.
            // Note that HPS transparency is determined by diffuse color alpha and the transmission channel
            // is for greyscale opacity texture. The transmission concept is not the same than in Luminate.

            RED::Color diffuseColor = materialInfo.colorsByChannel[ColorChannels::DiffuseColor];
            if (diffuseColor.A() != 1.0f) {
                float alpha = diffuseColor.A();
                materialInfo.colorsByChannel[ColorChannels::TransmissionColor] = RED::Color(1.0f - alpha);
            }

            // Retrieve the segment diffuse texture name if it exists
            if (getFacesTexture(a_segmentKeyPath,
                                HPS::Material::Channel::DiffuseTexture,
                                materialInfo.textureNameByChannel[TextureChannels::DiffuseTexture])) {
                registerTexture(a_segmentKeyPath,
                                materialInfo.textureNameByChannel[TextureChannels::DiffuseTexture].textureName,
                                a_resourceManager,
                                a_ioImageNameToLuminateMap,
                                a_ioTextureNameImageNameMap,
                                redImage);
            }

            // Determine bump map
            if (getFacesTexture(a_segmentKeyPath,
                                HPS::Material::Channel::Bump,
                                materialInfo.textureNameByChannel[TextureChannels::BumpTexture])) {
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
                                     HPS::ShellKey a_key,
                                     const RED::State& a_state,
                                     int a_point_count,
                                     RED::IMeshShape* a_imesh)
    {
        switch (uvParam) {
            case UVParameterization::World: {
                float* uvs = nullptr;
                float* fPoints = nullptr;

                uvs = new float[a_point_count * 2];

                RC_CHECK(a_imesh->GetArray((void const*&)fPoints, RED::MCL_VERTEX)); // points should be in world coordinates

                for (int i = 0; i < a_point_count; i++) {
                    uvs[2 * i] = *(fPoints + 3 * i);
                    uvs[2 * i + 1] = *(fPoints + 3 * i + 1);
                }

                RC_CHECK(a_imesh->SetArray(channel, uvs, a_point_count, 2, RED::MFT_FLOAT, a_state));
                delete[] uvs;
            } break;
            case UVParameterization::Cylinder:
                RC_CHECK(a_imesh->BuildTextureCoordinates(channel, RED::MTCM_CYLINDRICAL_CAP, RED::Matrix::IDENTITY, a_state));
                break;
            case UVParameterization::Sphere:
                RC_CHECK(a_imesh->BuildTextureCoordinates(channel, RED::MTCM_SPHERICAL, RED::Matrix::IDENTITY, a_state));
                break;
            default: {
                HPS::BoolArray validities;
                HPS::FloatArray uvs;

                if (a_key.ShowVertexParameters(validities, uvs)) {
                    int uvDimensions = (int)uvs.size() / a_point_count;

                    if (uvDimensions == 2 || uvDimensions == 3) {
                        for (int i = 0; i < (a_point_count * uvDimensions); i += uvDimensions) {
                            uvs[i + 1] = 1.f - uvs[i + 1]; // Y
                        }

                        RC_CHECK(a_imesh->SetArray(channel, uvs.data(), a_point_count, uvDimensions, RED::MFT_FLOAT, a_state));
                    }
                }
                else // Luminate need UV coordinates so we will build them
                {
                    RC_CHECK(a_imesh->BuildTextureCoordinates(channel, RED::MTCM_BOX, RED::Matrix::IDENTITY, a_state));
                }
            } break;
        }

        return RED_OK;
    }

    RED_RC convertVertexData(HPS::ShellKey a_key,
                             const RED::State& a_state,
                             int a_point_count,
                             RED::IMeshShape* a_imesh,
                             float* a_normals,
                             RealisticMaterialInfo const& a_material)
    {
        // Normals are needed if UV creation is called
        RC_TEST(setVertexNormals(a_state, a_imesh, a_point_count, a_normals));

        UVParameterization diffuseUVParam =
            a_material.textureNameByChannel[TextureChannels::DiffuseTexture].textureParameterization;
        computeUVParameterization(RED::MESH_CHANNEL::MCL_TEX0, diffuseUVParam, a_key, a_state, a_point_count, a_imesh);

        UVParameterization bumpUVParam = a_material.textureNameByChannel[TextureChannels::BumpTexture].textureParameterization;
        if (bumpUVParam != diffuseUVParam) {
            computeUVParameterization(RED::MESH_CHANNEL::MCL_TEX1, bumpUVParam, a_key, a_state, a_point_count, a_imesh);
        }

        // Create the vertex information used for shading tangent-space generation.
        // The channels mentionned here must match the SetupRealisticMaterial bump channels.
        // We reserve MCL_TEX0 for 3DF texture coordinates (if any, so use another channel)
        RC_CHECK(a_imesh->BuildTextureCoordinates(RED::MCL_TEX7, RED::MTCM_BOX, RED::Matrix::IDENTITY, a_state));
        RC_CHECK(a_imesh->BuildTangents(RED::MCL_USER0, RED::MCL_TEX7, a_state));

        return RED_OK;
    }

    RED::Object* convertHPSShellTrisStripsToREDMeshShape(HPS::ShellKey a_key,
                                                         RealisticMaterialInfo const& a_material,
                                                         const RED::State& a_state)
    {
        RED::Object* shape = nullptr;

        HPS::IntArray tristrips;
        HPS::IntArray faceIndices;

        if (a_key.ShowTristrips(tristrips)) {
            HPS::VectorArray vertexNormals;
            HPS::PointArray points;
            HPS::BoolArray vertexNormalValidities;

            if (a_key.ShowPoints(points)) {
                float* normalsArray = nullptr;

                if (a_key.ShowNetVertexNormals(vertexNormals))
                    normalsArray = &(vertexNormals.data()[0].x);

                shape = createREDMeshShapeFromTriStrips(
                    a_state, (int)tristrips.size(), tristrips.data(), (int)points.size(), (float*)points.data(), normalsArray);

                if (shape) {
                    convertVertexData(a_key, a_state, (int)points.size(), shape->As<RED::IMeshShape>(), normalsArray, a_material);
                }
            }
        }

        return shape;
    }

    HPS::UTF8 serializeKeyPath(const HPS::KeyPath& a_ioKeyPath)
    {
        HPS::UTF8 serialized = "";
        bool first = true;

        for (HPS::KeyPath::const_iterator it = a_ioKeyPath.cbegin(); it != a_ioKeyPath.cend(); ++it) {
            const HPS::Key& key = *it;

            if (first)
                first = false;
            else
                serialized += ",";

            if (!key.Empty()) {
                HPS::Type type = key.Type();
                HPS::UTF8 test, test3;
                switch (type) {
                    case HPS::Type::SegmentKey:
                        test = static_cast<HPS::SegmentKey>(key).Name();
                        serialized += test;
                        break;
                    case HPS::Type::ApplicationWindowKey:
                        test3 = static_cast<HPS::ApplicationWindowKey>(key).Name();
                        serialized += test3;
                        break;
                    default:
                        serialized += std::to_string(key.GetInstanceID()).c_str();
                        break;
                }
            }
            else
                serialized += " ";
        }
        return serialized;
    }

    RED::Object* convertSegmentTree(RED::Object* a_resourceManager,
                                    ConversionContextHPS& a_ioConversionContext,
                                    HPS::SegmentKey a_segmentKey,
                                    HPS::KeyPath& a_ioKeyPath)
    {
        RED::IResourceManager* iresourceManager = a_resourceManager->As<RED::IResourceManager>();

        //////////////////////////////////////////
        // Skip invisible segment conversion
        //////////////////////////////////////////

        if (!isSegmentVisible(a_ioKeyPath))
            return nullptr;

        //////////////////////////////////////////
        // Retrieve segment transform matrix it has one.
        // We want here the individual matrix, not the net one
        // because we benifit of Luminate transform stack as well.
        //////////////////////////////////////////

        RED::Matrix redMatrix = RED::Matrix::IDENTITY;
        HPS::MatrixKit modellingMatrix;

        if (a_segmentKey.ShowModellingMatrix(modellingMatrix)) {
            float vizMatrix[16];
            modellingMatrix.ShowElements(vizMatrix);
            redMatrix.SetColumnMajorMatrix(vizMatrix);
        }

        //////////////////////////////////////////
        // Search and convert supported segment geometry
        //////////////////////////////////////////

        HPS::SearchOptionsKit searchOptionsKit;

        HPS::SearchTypeArray geometrySearchTypeArray = {HPS::Search::Type::Cylinder,
                                                        HPS::Search::Type::Mesh,
                                                        HPS::Search::Type::Shell,
                                                        HPS::Search::Type::Sphere,
                                                        HPS::Search::Type::Polygon};

        searchOptionsKit.SetCriteria(geometrySearchTypeArray);
        searchOptionsKit.SetSearchSpace(HPS::Search::Space::SegmentOnly);
        searchOptionsKit.SetBehavior(HPS::Search::Behavior::Exhaustive);

        HPS::SearchResults searchResults;
        size_t count = a_segmentKey.Find(searchOptionsKit, searchResults);

        bool hasGeometry = count > 0;
        RED::Object* material = nullptr;
        std::vector<RED::Object*> meshShapes;

        // Serialize keyPath for use in maps
        HPS::UTF8 serializedKeyPath = serializeKeyPath(a_ioKeyPath);

        if (hasGeometry) {
            // Create a RED material only for segment with geometries and we build it along its full key path to manage correctly
            // attributes and includes.

            RealisticMaterialInfo materialInfo = getSegmentMaterialInfo(a_ioKeyPath,
                                                                        a_resourceManager,
                                                                        a_ioConversionContext.defaultMaterialInfo,
                                                                        a_ioConversionContext.imageNameToLuminateMap,
                                                                        a_ioConversionContext.textureNameImageNameMap,
                                                                        a_ioConversionContext.pbrToRealisticConversionMap);

            // If the geometry is set to invisible (diffuse color alpha to 0), we can skip it.

            if (materialInfo.colorsByChannel[ColorChannels::DiffuseColor].A() == 0.)
                return nullptr;

            material = createREDMaterial(materialInfo,
                                         a_resourceManager,
                                         a_ioConversionContext.imageNameToLuminateMap,
                                         a_ioConversionContext.textureNameImageNameMap,
                                         a_ioConversionContext.materials);

            // Check if the geometry of the current segment has already be converted, thus it can be shared.
            SegmentMeshShapesMap::iterator segmentMeshShapesIt =
                a_ioConversionContext.segmentMeshShapesMap.find(a_segmentKey.GetInstanceID());

            if (segmentMeshShapesIt != a_ioConversionContext.segmentMeshShapesMap.end()) {
                meshShapes = segmentMeshShapesIt->second;
            }
            else {
                // Convert each geometry.

                HPS::SearchResultsIterator searchResultIterator = searchResults.GetIterator();

                while (searchResultIterator.IsValid()) {
                    RED::Object* shape = nullptr;

                    HPS::Key itemKey = searchResultIterator.GetItem();
                    HPS::Type type = itemKey.Type();

                    if (type == HPS::Type::ShellKey) {
                        HPS::ShellKey shellKey(itemKey);
                        shape = convertHPSShellTrisStripsToREDMeshShape(shellKey, materialInfo, iresourceManager->GetState());
                    }
                    else if (type == HPS::Type::PolygonKey) {
                        HPS::PolygonKey polygonKey(itemKey);
                        HPS::SegmentKey tempSegmentKey = HPS::Database::CreateRootSegment();
                        HPS::ShellKey tempShellKey = tempSegmentKey.InsertShellFromGeometry(polygonKey);
                        shape = convertHPSShellTrisStripsToREDMeshShape(tempShellKey, materialInfo, iresourceManager->GetState());
                        tempShellKey.Delete();
                    }
                    else if (type == HPS::Type::MeshKey) {
                        HPS::MeshKey meshKey(itemKey);
                        HPS::SegmentKey tempSegmentKey = HPS::Database::CreateRootSegment();
                        HPS::ShellKey tempShellKey = tempSegmentKey.InsertShellFromGeometry(meshKey);
                        shape = convertHPSShellTrisStripsToREDMeshShape(tempShellKey, materialInfo, iresourceManager->GetState());
                        tempShellKey.Delete();
                    }
                    else if (type == HPS::Type::SphereKey) {
                        HPS::SphereKey sphereKey(itemKey);
                        HPS::SegmentKey tempSegmentKey = HPS::Database::CreateRootSegment();
                        HPS::ShellKey tempShellKey = tempSegmentKey.InsertShellFromGeometry(sphereKey);
                        shape = convertHPSShellTrisStripsToREDMeshShape(tempShellKey, materialInfo, iresourceManager->GetState());
                        tempShellKey.Delete();
                    }
                    else if (type == HPS::Type::CylinderKey) {
                        HPS::CylinderKey cylinderKey(itemKey);
                        HPS::SegmentKey tempSegmentKey = HPS::Database::CreateRootSegment();
                        HPS::ShellKey tempShellKey = tempSegmentKey.InsertShellFromGeometry(cylinderKey);
                        shape = convertHPSShellTrisStripsToREDMeshShape(tempShellKey, materialInfo, iresourceManager->GetState());
                        tempShellKey.Delete();
                    }

                    if (shape != nullptr)
                        meshShapes.push_back(shape);

                    searchResultIterator.Next();
                }

                a_ioConversionContext.segmentMeshShapesMap[a_segmentKey.GetInstanceID()] = meshShapes;
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

        searchOptionsKit.SetCriteria({HPS::Search::Type::Segment});
        count = a_segmentKey.Find(searchOptionsKit, searchResults);

        if (count > 0) {
            HPS::SearchResultsIterator searchResultIterator = searchResults.GetIterator();

            while (searchResultIterator.IsValid()) {
                HPS::Key itemKey = searchResultIterator.GetItem();
                HPS::SegmentKey segmentKey(itemKey);

                a_ioKeyPath.PushFront(segmentKey);
                RED::Object* child = convertSegmentTree(a_resourceManager, a_ioConversionContext, segmentKey, a_ioKeyPath);
                a_ioKeyPath.PopFront();

                if (child)
                    itransform->AddChild(child, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());

                searchResultIterator.Next();
            }
        }

        //////////////////////////////////////////
        // Process segment includes.
        // Due to attributes propagation in 3DF, we cannot
        // share the full include subgraph but we have to duplicate
        // it in order to apply different materials in Luminate.
        // Thus a segment include acts like a child but we
        // store the include key in the keypath.
        //////////////////////////////////////////

        searchOptionsKit.SetCriteria({HPS::Search::Type::Include});
        count = a_segmentKey.Find(searchOptionsKit, searchResults);

        if (count > 0) {
            HPS::SearchResultsIterator searchResultIterator = searchResults.GetIterator();

            while (searchResultIterator.IsValid()) {
                HPS::Key itemKey = searchResultIterator.GetItem();
                HPS::IncludeKey includeKey(itemKey);
                HPS::SegmentKey targetSegmentKey = includeKey.GetTarget();

                a_ioKeyPath.PushFront(includeKey);
                a_ioKeyPath.PushFront(targetSegmentKey);
                RED::Object* child = convertSegmentTree(a_resourceManager, a_ioConversionContext, targetSegmentKey, a_ioKeyPath);
                a_ioKeyPath.PopFront();
                a_ioKeyPath.PopFront();

                if (child)
                    itransform->AddChild(child, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());

                searchResultIterator.Next();
            }
        }

        return transform;
    }

    LuminateSceneInfoPtr convertHPSSceneToLuminate(HPS::View* a_hpsView)
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

        Handedness viewHandedness = getViewHandedness(a_hpsView);

        if (viewHandedness == Handedness::LeftHanded) {
            RC_CHECK(itransform->SetMatrix(&HoopsLuminateBridge::s_leftHandedToRightHandedMatrix, iresourceManager->GetState()));
        }

        //////////////////////////////////////////
        // Create the default material definition.
        //////////////////////////////////////////

        RealisticMaterialInfo defaultMaterialInfo = createDefaultRealisticMaterialInfo();

        //////////////////////////////////////////
        // Initialize the conversion context which will hold
        // current conversion data state during conversion.
        //////////////////////////////////////////

        ConversionContextHPSPtr sceneInfoPtr = std::make_shared<ConversionContextHPS>();
        sceneInfoPtr->rootTransformShape = rootTransformShape;
        sceneInfoPtr->defaultMaterialInfo = defaultMaterialInfo;
        sceneInfoPtr->viewHandedness = viewHandedness;

        //////////////////////////////////////////
        // Proceed with segment tree traversal to convert scene
        //////////////////////////////////////////

        HPS::SegmentKey modelSegmentKey = a_hpsView->GetAttachedModel().GetSegmentKey();

        HPS::KeyPath segmentKeyPath;
        segmentKeyPath.PushFront(modelSegmentKey);
        RED::Object* result = convertSegmentTree(resourceManager, *sceneInfoPtr, modelSegmentKey, segmentKeyPath);
        segmentKeyPath.PopFront();

        if (result)
            itransform->AddChild(result, RED_SHP_DAG_NO_UPDATE, iresourceManager->GetState());

        return sceneInfoPtr;
    }
} // namespace hoops_luminate_bridge
