#ifndef LUMINATEBRIDGE_HOOPSHPSLUMINATEBRIDGE_H
#define LUMINATEBRIDGE_HOOPSHPSLUMINATEBRIDGE_H

#include <map>

#include "sprk.h"

#include <hoops_luminate_bridge/HoopsLuminateBridge.h>
#include <hoops_luminate_bridge/ConversionTools.h>

namespace hoops_luminate_bridge {

    /**
     * HPS::UTF8 comparator in order to use it as map key.
     */
    struct HPSUTF8Comparator {
        bool operator()(HPS::UTF8 const& lhs, HPS::UTF8 const& rhs) const
        {
            char const* lchars = lhs.GetBytes();
            char const* rchars = rhs.GetBytes();
            return std::string(lchars) < std::string(rchars);
        }
    };

    /**
     * Mapping between segment hash and associated Luminate mesh shapes.
     */
    using SegmentMeshShapesMap = std::map<intptr_t, std::vector<RED::Object*>>;

    /**
     * Mapping between segment hash and associated Luminate transform shape.
     */
    using SegmentTransformShapeMap = std::map<HPS::UTF8, RED::Object*, HPSUTF8Comparator>;

    /**
     * LuminateSceneInfo extension with specific HPS informations.
     */
    struct ConversionContextHPS: LuminateSceneInfo {
        SegmentMeshShapesMap segmentMeshShapesMap;
        SegmentTransformShapeMap segmentTransformShapeMap;
    };

    using ConversionContextHPSPtr = std::shared_ptr<ConversionContextHPS>;

    /**
     * Structure storing the information about selected segment.
     */
    struct SelectedHPSSegmentInfo: SelectedSegmentInfo {
        intptr_t m_instanceID;
        HPS::UTF8 m_serializedKeyPath;
        HPS::SegmentKey m_segmentKey;

        SelectedHPSSegmentInfo(intptr_t a_instanceID,
                               HPS::SegmentKey const& a_segmentKey,
                               HPS::UTF8 const& a_serializedKeyPath,
                               RED::Color const& a_diffuseColor):
            SelectedSegmentInfo(a_diffuseColor), m_instanceID(a_instanceID), m_segmentKey(a_segmentKey), m_serializedKeyPath(a_serializedKeyPath){};
    };
    using SelectedHPSSegmentInfoPtr = std::shared_ptr<SelectedHPSSegmentInfo>;

    /**
     * \brief HPS KeyPath to string serialization.
     * @param[in] a_ioKeyPath HPS KeyPath to serialize.
     * @return Serialized KeyPath.
     */
    HPS::UTF8 serializeKeyPath(const HPS::KeyPath& a_ioKeyPath);

    class HoopsLuminateBridgeHPS: public HoopsLuminateBridge {
        // Instance Fields
        // ---------------

      private:
        HPS::View* m_hpsView;
        HPS::CameraKit m_hpsCamera;

        // Constructors
        // ------------

      public:
        HoopsLuminateBridgeHPS(HPS::View* a_hpsView);
        ~HoopsLuminateBridgeHPS();

        // Static Methods
        // --------------

      public:
        static bool getFacesColor(const HPS::KeyPath& a_segmentKeyPath,
                           HPS::Material::Channel a_channel,
                           RED::Color& a_oColor,
                           std::string& a_oTextureName);

        // Virtual Methods Impl
        // --------------------

      private:
        void saveCameraState() override;
        LuminateSceneInfoPtr convertScene() override;
        bool checkCameraChange() override;
        CameraInfo getCameraInfo() override;
        void updateSelectedSegmentTransform() override;
        std::vector<RED::Object*> getSelectedLuminateMeshShapes() override;
        RED::Object* getSelectedLuminateTransformNode() override;
        RED_RC syncRootTransform() override;
    };

    /**
     * \brief HPS scene to Luminate conversion entry point.
     * @param[in] a_hpsView HPS view currently displaying the model to convert.
     * @return Converted Luminate scene informations.
     */
    LuminateSceneInfoPtr convertHPSSceneToLuminate(HPS::View* a_hpsView);

    /**
     * \brief Gets generic camera informations from the current HPS camera.
     * @param[in] a_hpsView HPS view owning the camera.
     * @return Generic camera informations.
     */
    CameraInfo getHPSCameraInfo(HPS::View* a_view);

} // namespace hoops_luminate_bridge

#endif
