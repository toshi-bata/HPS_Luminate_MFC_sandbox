#include "KeyPathUtils.h"
#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>

namespace KeyPathUtils {
    HPS::UTF8 serializeRawKeyPath(HPS::KeyPath const& a_keyPath, HPS::SegmentKey const& a_modelRootSegmentKey)
    {
        // Remove all segments before the model root segment to have a
        // keypath at model level.

        HPS::KeyPath keyPath = a_keyPath;

        if (!a_keyPath.Empty()) {
            bool found = false;
            for (int i = (int)keyPath.Size() - 1; !found && i >= 0; --i) {
                if (keyPath.At(i).Equals(a_modelRootSegmentKey)) {
                    found = true;
                }
            }

            if (found) {
                while (!keyPath.Back().Equals(a_modelRootSegmentKey))
                    keyPath.PopBack();
            }
        }

        // Perform serialization.

        HPS::UTF8 serializedKeyPath = hoops_luminate_bridge::serializeKeyPath(keyPath);
        return serializedKeyPath;
    }
} // namespace KeyPathUtils