#pragma once

#include "monitorlayout.h"

#include <DisplayControl.h>

namespace KHeadless
{

KRdp::DisplayMonitorList toKrdpLayout(const MonitorLayout &layout);
MonitorLayout fromKrdpLayout(const KRdp::DisplayMonitorList &layout);

}
