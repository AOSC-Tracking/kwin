/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Xaver Hugl <xaver.hugl@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QSharedPointer>

namespace KWin
{

class DrmPipelineLayer;
class DrmDisplayDevice;
class DrmVirtualOutput;
class DrmPipeline;
class DrmOutputLayer;

class DrmRenderBackend
{
public:
    virtual ~DrmRenderBackend() = default;

    virtual QSharedPointer<DrmPipelineLayer> createDrmPipelineLayer(DrmPipeline *pipeline) = 0;
    virtual QSharedPointer<DrmOutputLayer> createLayer(DrmVirtualOutput *output) = 0;

};

}
