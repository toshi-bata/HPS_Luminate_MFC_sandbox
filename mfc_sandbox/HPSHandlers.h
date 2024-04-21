#ifndef HPS_HANDLERS_OPERATORS_H
#define HPS_HANDLERS_OPERATORS_H

#include <stdio.h>
#include "hps.h"
#include "CHPSDoc.h"
#include "CHPSView.h"
#include <hoops_luminate_bridge/HoopsHPSLuminateBridge.h>

using namespace hoops_luminate_bridge;

class SegmentSelectedHandler: public HPS::EventHandler {
  private:
    CHPSView* m_mHpsView;
    HoopsLuminateBridge* m_luminateBridge;

  public:
    SegmentSelectedHandler();
    virtual ~SegmentSelectedHandler();

    void setView(CHPSView* a_view, HoopsLuminateBridge* a_bridge);
    HandleResult Handle(HPS::Event const* in_event) override;
};

#endif