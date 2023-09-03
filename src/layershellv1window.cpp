/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "layershellv1window.h"
#include "core/output.h"
#include "layershellv1integration.h"
#include "screenedge.h"
#include "wayland/layershell_v1_interface.h"
#include "wayland/output_interface.h"
#include "wayland/screenedge_v1_interface.h"
#include "wayland/surface_interface.h"
#include "wayland_server.h"
#include "workspace.h"

using namespace KWaylandServer;

namespace KWin
{

static NET::WindowType scopeToType(const QString &scope)
{
    static const QHash<QString, NET::WindowType> scopeToType{
        {QStringLiteral("desktop"), NET::Desktop},
        {QStringLiteral("dock"), NET::Dock},
        {QStringLiteral("crititical-notification"), NET::CriticalNotification},
        {QStringLiteral("notification"), NET::Notification},
        {QStringLiteral("tooltip"), NET::Tooltip},
        {QStringLiteral("on-screen-display"), NET::OnScreenDisplay},
        {QStringLiteral("dialog"), NET::Dialog},
        {QStringLiteral("splash"), NET::Splash},
        {QStringLiteral("utility"), NET::Utility},
    };
    return scopeToType.value(scope.toLower(), NET::Normal);
}

LayerShellV1Window::LayerShellV1Window(LayerSurfaceV1Interface *shellSurface,
                                       Output *output,
                                       LayerShellV1Integration *integration)
    : WaylandWindow(shellSurface->surface())
    , m_desiredOutput(output)
    , m_integration(integration)
    , m_shellSurface(shellSurface)
    , m_windowType(scopeToType(shellSurface->scope()))
{
    setSkipSwitcher(!isDesktop());
    setSkipPager(true);
    setSkipTaskbar(true);

    connect(shellSurface, &LayerSurfaceV1Interface::aboutToBeDestroyed,
            this, &LayerShellV1Window::destroyWindow);
    connect(shellSurface->surface(), &SurfaceInterface::aboutToBeDestroyed,
            this, &LayerShellV1Window::destroyWindow);

    connect(output, &Output::geometryChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(output, &Output::enabledChanged,
            this, &LayerShellV1Window::handleOutputEnabledChanged);

    connect(shellSurface->surface(), &SurfaceInterface::unmapped,
            this, &LayerShellV1Window::handleUnmapped);
    connect(shellSurface->surface(), &SurfaceInterface::committed,
            this, &LayerShellV1Window::handleCommitted);

    connect(shellSurface, &LayerSurfaceV1Interface::configureAcknowledged,
            this, &LayerShellV1Window::handleConfigureAcknowledged);
    connect(shellSurface, &LayerSurfaceV1Interface::desiredSizeChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(shellSurface, &LayerSurfaceV1Interface::layerChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(shellSurface, &LayerSurfaceV1Interface::marginsChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(shellSurface, &LayerSurfaceV1Interface::anchorChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(shellSurface, &LayerSurfaceV1Interface::exclusiveZoneChanged,
            this, &LayerShellV1Window::scheduleRearrange);
    connect(shellSurface, &LayerSurfaceV1Interface::acceptsFocusChanged,
            this, &LayerShellV1Window::handleAcceptsFocusChanged);

    m_configureTimer.setSingleShot(true);
    connect(&m_configureTimer, &QTimer::timeout, this, &LayerShellV1Window::sendConfigure);
}

LayerSurfaceV1Interface *LayerShellV1Window::shellSurface() const
{
    return m_shellSurface;
}

Output *LayerShellV1Window::desiredOutput() const
{
    return m_desiredOutput;
}

void LayerShellV1Window::scheduleRearrange()
{
    m_integration->scheduleRearrange();
}

NET::WindowType LayerShellV1Window::windowType(bool) const
{
    return m_windowType;
}

bool LayerShellV1Window::isPlaceable() const
{
    return false;
}

bool LayerShellV1Window::isCloseable() const
{
    return true;
}

bool LayerShellV1Window::isMovable() const
{
    return false;
}

bool LayerShellV1Window::isMovableAcrossScreens() const
{
    return false;
}

bool LayerShellV1Window::isResizable() const
{
    return false;
}

bool LayerShellV1Window::takeFocus()
{
    setActive(true);
    return true;
}

bool LayerShellV1Window::wantsInput() const
{
    return acceptsFocus() && readyForPainting();
}

StrutRect LayerShellV1Window::strutRect(StrutArea area) const
{
    switch (area) {
    case StrutAreaLeft:
        if (m_shellSurface->exclusiveEdge() == Qt::LeftEdge) {
            return StrutRect(m_moveResizeGeometry.x(),
                             m_moveResizeGeometry.y(),
                             m_shellSurface->exclusiveZone(),
                             m_moveResizeGeometry.height(),
                             StrutAreaLeft);
        }
        return StrutRect();
    case StrutAreaRight:
        if (m_shellSurface->exclusiveEdge() == Qt::RightEdge) {
            return StrutRect(m_moveResizeGeometry.x() + m_moveResizeGeometry.width() - m_shellSurface->exclusiveZone(),
                             m_moveResizeGeometry.y(),
                             m_shellSurface->exclusiveZone(),
                             m_moveResizeGeometry.height(),
                             StrutAreaRight);
        }
        return StrutRect();
    case StrutAreaTop:
        if (m_shellSurface->exclusiveEdge() == Qt::TopEdge) {
            return StrutRect(m_moveResizeGeometry.x(),
                             m_moveResizeGeometry.y(),
                             m_moveResizeGeometry.width(),
                             m_shellSurface->exclusiveZone(),
                             StrutAreaTop);
        }
        return StrutRect();
    case StrutAreaBottom:
        if (m_shellSurface->exclusiveEdge() == Qt::BottomEdge) {
            return StrutRect(m_moveResizeGeometry.x(),
                             m_moveResizeGeometry.y() + m_moveResizeGeometry.height() - m_shellSurface->exclusiveZone(),
                             m_moveResizeGeometry.width(),
                             m_shellSurface->exclusiveZone(),
                             StrutAreaBottom);
        }
        return StrutRect();
    default:
        return StrutRect();
    }
}

bool LayerShellV1Window::hasStrut() const
{
    return m_shellSurface->exclusiveZone() > 0;
}

void LayerShellV1Window::destroyWindow()
{
    m_configureTimer.stop();

    if (m_screenEdge) {
        m_screenEdge->disconnect(this);
    }
    m_shellSurface->disconnect(this);
    m_shellSurface->surface()->disconnect(this);
    m_desiredOutput->disconnect(this);

    markAsDeleted();
    cleanTabBox();
    Q_EMIT closed();
    StackingUpdatesBlocker blocker(workspace());
    cleanGrouping();
    waylandServer()->removeWindow(this);
    scheduleRearrange();
    unref();
}

void LayerShellV1Window::closeWindow()
{
    m_shellSurface->sendClosed();
}

Layer LayerShellV1Window::belongsToLayer() const
{
    switch (m_shellSurface->layer()) {
    case LayerSurfaceV1Interface::BackgroundLayer:
        return DesktopLayer;
    case LayerSurfaceV1Interface::BottomLayer:
        return BelowLayer;
    case LayerSurfaceV1Interface::TopLayer:
        return AboveLayer;
    case LayerSurfaceV1Interface::OverlayLayer:
        return UnmanagedLayer;
    default:
        Q_UNREACHABLE();
    }
}

bool LayerShellV1Window::acceptsFocus() const
{
    return m_shellSurface->acceptsFocus();
}

void LayerShellV1Window::moveResizeInternal(const QRectF &rect, MoveResizeMode mode)
{
    if (areGeometryUpdatesBlocked()) {
        setPendingMoveResizeMode(mode);
        return;
    }

    const QSizeF requestedClientSize = frameSizeToClientSize(rect.size());
    if (m_configureEvents.isEmpty() && requestedClientSize == clientSize()) {
        updateGeometry(rect);
    } else {
        scheduleConfigure();
    }
}

void LayerShellV1Window::handleConfigureAcknowledged(quint32 serial)
{
    while (!m_configureEvents.isEmpty()) {
        m_lastConfigureEvent = m_configureEvents.takeFirst();
        if (m_lastConfigureEvent.serial == serial) {
            break;
        }
    }
}

void LayerShellV1Window::handleUnmapped()
{
    m_integration->recreateWindow(shellSurface());
}

void LayerShellV1Window::handleCommitted()
{
    if (!surface()->buffer()) {
        return;
    }

    updateGeometry(QRectF(m_lastConfigureEvent.position, clientSizeToFrameSize(surface()->size())));
    markAsMapped();
}

void LayerShellV1Window::handleAcceptsFocusChanged()
{
    switch (m_shellSurface->layer()) {
    case LayerSurfaceV1Interface::TopLayer:
    case LayerSurfaceV1Interface::OverlayLayer:
        if (wantsInput()) {
            workspace()->activateWindow(this);
        }
        break;
    case LayerSurfaceV1Interface::BackgroundLayer:
    case LayerSurfaceV1Interface::BottomLayer:
        break;
    }
}

void LayerShellV1Window::handleOutputEnabledChanged()
{
    if (!m_desiredOutput->isEnabled()) {
        closeWindow();
        destroyWindow();
    }
}

void LayerShellV1Window::setVirtualKeyboardGeometry(const QRectF &geo)
{
    if (m_virtualKeyboardGeometry == geo) {
        return;
    }

    m_virtualKeyboardGeometry = geo;
    scheduleRearrange();
}

void LayerShellV1Window::showOnScreenEdge()
{
    // ShowOnScreenEdge can be called by an Edge, and setHidden could destroy the Edge
    // Use the singleshot to avoid use-after-free
    QTimer::singleShot(0, this, &LayerShellV1Window::deactivateScreenEdge);
}

void LayerShellV1Window::installAutoHideScreenEdgeV1(KWaylandServer::AutoHideScreenEdgeV1Interface *edge)
{
    m_screenEdge = edge;

    connect(edge, &KWaylandServer::AutoHideScreenEdgeV1Interface::destroyed,
            this, &LayerShellV1Window::deactivateScreenEdge);
    connect(edge, &KWaylandServer::AutoHideScreenEdgeV1Interface::activateRequested,
            this, &LayerShellV1Window::activateScreenEdge);
    connect(edge, &KWaylandServer::AutoHideScreenEdgeV1Interface::deactivateRequested,
            this, &LayerShellV1Window::deactivateScreenEdge);

    connect(this, &LayerShellV1Window::frameGeometryChanged, edge, [this]() {
        if (m_screenEdgeActive) {
            reserveScreenEdge();
        }
    });
}

void LayerShellV1Window::reserveScreenEdge()
{
    if (workspace()->screenEdges()->reserve(this, m_screenEdge->border())) {
        setHidden(true);
    } else {
        setHidden(false);
    }
}

void LayerShellV1Window::unreserveScreenEdge()
{
    setHidden(false);
    workspace()->screenEdges()->reserve(this, ElectricNone);
}

void LayerShellV1Window::activateScreenEdge()
{
    m_screenEdgeActive = true;
    reserveScreenEdge();
}

void LayerShellV1Window::deactivateScreenEdge()
{
    m_screenEdgeActive = false;
    unreserveScreenEdge();
}

void LayerShellV1Window::scheduleConfigure()
{
    m_configureTimer.start();
}

void LayerShellV1Window::sendConfigure()
{
    const quint32 serial = m_shellSurface->sendConfigure(frameSizeToClientSize(moveResizeGeometry().size()).toSize());
    m_configureEvents.append(LayerShellV1ConfigureEvent{
        .position = moveResizeGeometry().topLeft(),
        .serial = serial,
    });
}

} // namespace KWin

#include "moc_layershellv1window.cpp"
