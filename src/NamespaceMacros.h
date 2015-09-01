/**
 * @file NamespaceMacros.h
 * @author  Qube Dev <QubeDev@realimage.com>

 * @copyright Copyright &copy; 2015 Qube Cinema Inc. All Rights reserved
 *
 * @brief
 * Defines namespace of KeySmithClient application
 */

#pragma once

#define KEY_SMITH_NS Qube::KeySmith
#define KEY_SMITH_NS_START                                                                            \
    namespace Qube                                                                                 \
    {                                                                                              \
        namespace KeySmith                                                                        \
        {
#define KEY_SMITH_NS_STOP                                                                             \
    }                                                                                              \
    }
