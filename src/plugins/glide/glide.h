/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Philip Falkner <philip.falkner@gmail.com>
    SPDX-FileCopyrightText: 2009 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2010 Alexandre Pereira <pereira.alex@gmail.com>
    SPDX-FileCopyrightText: 2018 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

// kwineffects
#include "effect/effect.h"
#include "effect/effectwindow.h"
#include "effect/timeline.h"

namespace KWin
{

struct GlideAnimation
{
    EffectWindowDeletedRef deletedRef;
    TimeLine timeLine;
};

class GlideEffect : public Effect
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration)
    Q_PROPERTY(RotationEdge inRotationEdge READ inRotationEdge)
    Q_PROPERTY(qreal inRotationAngle READ inRotationAngle)
    Q_PROPERTY(qreal inDistance READ inDistance)
    Q_PROPERTY(qreal inOpacity READ inOpacity)
    Q_PROPERTY(RotationEdge outRotationEdge READ outRotationEdge)
    Q_PROPERTY(qreal outRotationAngle READ outRotationAngle)
    Q_PROPERTY(qreal outDistance READ outDistance)
    Q_PROPERTY(qreal outOpacity READ outOpacity)

public:
    GlideEffect();
    ~GlideEffect() override;

    void reconfigure(ReconfigureFlags flags) override;

    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, QRegion region, WindowPaintData &data) override;
    void postPaintScreen() override;

    bool isActive() const override;
    int requestedEffectChainPosition() const override;

    static bool supported();

    enum RotationEdge {
        Top = 0,
        Right = 1,
        Bottom = 2,
        Left = 3
    };
    Q_ENUM(RotationEdge)

    int duration() const;
    RotationEdge inRotationEdge() const;
    qreal inRotationAngle() const;
    qreal inDistance() const;
    qreal inOpacity() const;
    RotationEdge outRotationEdge() const;
    qreal outRotationAngle() const;
    qreal outDistance() const;
    qreal outOpacity() const;

private Q_SLOTS:
    void windowAdded(EffectWindow *w);
    void windowClosed(EffectWindow *w);
    void windowDataChanged(EffectWindow *w, int role);

private:
    bool isGlideWindow(EffectWindow *w) const;

    std::chrono::milliseconds m_duration;
    QHash<EffectWindow *, GlideAnimation> m_animations;

    struct GlideParams
    {
        RotationEdge edge;
        struct
        {
            qreal from;
            qreal to;
        } angle, distance, opacity;
    };

    GlideParams m_inParams;
    GlideParams m_outParams;
};

inline int GlideEffect::requestedEffectChainPosition() const
{
    return 50;
}

inline int GlideEffect::duration() const
{
    return m_duration.count();
}

inline GlideEffect::RotationEdge GlideEffect::inRotationEdge() const
{
    return m_inParams.edge;
}

inline qreal GlideEffect::inRotationAngle() const
{
    return m_inParams.angle.from;
}

inline qreal GlideEffect::inDistance() const
{
    return m_inParams.distance.from;
}

inline qreal GlideEffect::inOpacity() const
{
    return m_inParams.opacity.from;
}

inline GlideEffect::RotationEdge GlideEffect::outRotationEdge() const
{
    return m_outParams.edge;
}

inline qreal GlideEffect::outRotationAngle() const
{
    return m_outParams.angle.to;
}

inline qreal GlideEffect::outDistance() const
{
    return m_outParams.distance.to;
}

inline qreal GlideEffect::outOpacity() const
{
    return m_outParams.opacity.to;
}

} // namespace KWin
