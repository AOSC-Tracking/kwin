/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Xaver Hugl <xaver.hugl@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "fakeoutput.h"

FakeOutput::FakeOutput()
{
    setMode(QSize(1024, 720), 60000);
}

KWin::RenderLoop *FakeOutput::renderLoop() const
{
    return nullptr;
}

void FakeOutput::setMode(QSize size, uint32_t refreshRate)
{
    auto mode = std::make_shared<KWin::OutputMode>(size, refreshRate);

    State state = m_state;
    state.modes = {mode};
    state.currentMode = mode;
    setState(state);
}

void FakeOutput::setTransform(Transform transform)
{
    State state = m_state;
    state.transform = transform;
    setState(state);
}

void FakeOutput::moveTo(const QPoint &pos)
{
    State state = m_state;
    state.position = pos;
    setState(state);
}

void FakeOutput::setScale(qreal scale)
{
    State state = m_state;
    state.scale = scale;
    setState(state);
}

void FakeOutput::setSubPixel(SubPixel subPixel)
{
    setInformation({
        .subPixel = subPixel,
    });
}

void FakeOutput::setDpmsSupported(bool supported)
{
    setInformation({
        .capabilities = supported ? Capability::Dpms : Capabilities(),
    });
}

void FakeOutput::setPhysicalSize(QSize size)
{
    setInformation({
        .physicalSize = size,
    });
}
