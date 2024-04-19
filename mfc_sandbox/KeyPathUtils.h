#ifndef KEYPATHUTILS_H
#define KEYPATHUTILS_H

#include <hps.h>

namespace KeyPathUtils {
    HPS::UTF8 serializeRawKeyPath(HPS::KeyPath const& a_keyPath, HPS::SegmentKey const& a_modelRootSegmentKey);
}

#endif