/**
 * @file NamespaceMacros.h

 * @copyright Copyright &copy; 2017 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Defines namespace of QubeWireClient application
 */

#pragma once

#define QUBE_WIRE_NS Qube::Wire
#define QUBE_WIRE_NS_START                                                                            \
  namespace Qube                                                                                 \
  {                                                                                              \
    namespace Wire                                                                        \
    {
#define QUBE_WIRE_NS_STOP                                                                             \
    }                                                                                              \
  }
