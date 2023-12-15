/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2017 Martin Flöser <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once
#if KWIN_BUILD_X11

#include "x11eventfilter.h"

namespace KWin
{
class RootInfo;

class RootInfoFilter : public X11EventFilter
{
public:
    explicit RootInfoFilter(RootInfo *parent);

    bool event(xcb_generic_event_t *event) override;

private:
    RootInfo *m_rootInfo;
};

}
#endif
