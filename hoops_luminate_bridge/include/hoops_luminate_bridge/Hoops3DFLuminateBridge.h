#ifndef LUMINATEBRIDGE_HOOPS3DFLUMINATEBRIDGE_H
#define LUMINATEBRIDGE_HOOPS3DFLUMINATEBRIDGE_H

#include <HBaseView.h>

#include <hoops_luminate_bridge/HoopsLuminateBridge.h>
#include <hoops_luminate_bridge/ConversionTools.h>

namespace hoops_luminate_bridge {

    /**
     * Mapping between segment key and associated Luminate mesh shapes.
     */
    using SegmentMeshShapesMap = std::map<HC_KEY, std::vector<RED::Object*>>;

    /**
     * Mapping between segment key and associated Luminate transform shape.
     */
    using SegmentTransformShapeMap = std::map<std::string, RED::Object*>;

    /**
     * LuminateSceneInfo extension with specific 3DF informations.
     */
    struct ConversionContext3DF: LuminateSceneInfo {
        SegmentMeshShapesMap segmentMeshShapesMap;
        SegmentTransformShapeMap segmentTransformShapeMap;
    };

    using ConversionContext3DFPtr = std::shared_ptr<ConversionContext3DF>;

    /**
     * \brief 3DF KeyPath to string serialization.
     * @param[in] a_ioKeyPath 3DF KeyPath to serialize.
     * @return Serialized KeyPath.
     */
    std::string serializeKeyPath(std::vector<HC_KEY> const& a_ioKeyPath); 
    
    /**
     * \brief Get the color of a segment.
     * @param[in] a_segmentKeyPath 3DF KeyPath to the segment.
     * @param[in] a_channel color channel to look for.
     * @param[out] a_oColor result color.
     * @param[out] a_oTextureName result texture.
     * @return Serialized KeyPath.
     */
    bool getFacesColor(std::vector<HC_KEY> const& a_segmentKeyPath,
                       std::string const& a_channel,
                       RED::Color& a_oColor,
                       std::string& a_oTextureName);
    
    /**
     * Structure storing the information about selected segment.
     */
    struct Selected3DFSegmentInfo : SelectedSegmentInfo {
        std::string m_serializedKeyPath;
        HC_KEY m_segmentKey;

        Selected3DFSegmentInfo(HC_KEY const& a_segmentKey,
                               std::string const& a_serializedKeyPath,
                               RED::Color const& a_diffuseColor):
            SelectedSegmentInfo(a_diffuseColor), m_segmentKey(a_segmentKey), m_serializedKeyPath(a_serializedKeyPath){};
    };
    using Selected3DFSegmentInfoPtr = std::shared_ptr<Selected3DFSegmentInfo>;

    class LuminateBridge3DF: public HoopsLuminateBridge {
        // Instance Fields
        // ---------------

      private:
        HBaseView* m_3dfView;
        HCamera m_3dfCamera;

        // Constructors
        // ------------

      public:
        LuminateBridge3DF(HBaseView* a_3dfView);
        ~LuminateBridge3DF();

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
     * \brief 3DF scene to Luminate conversion entry point.
     * @param[in] a_3dfView 3DF base view currently displaying the model to convert.
     * @return Converted Luminate scene informations.
     */
    LuminateSceneInfoPtr convert3DFSceneToLuminate(HBaseView* a_3dfView);

    /**
     * \brief Gets generic camera informations from the current 3DF camera.
     * @param[in] a_3dfView 3DF base view owning the camera.
     * @return Generic camera informations.
     */
    CameraInfo get3DFCameraInfo(HBaseView* a_3dfView);

} // namespace hoops_luminate_bridge

#endif