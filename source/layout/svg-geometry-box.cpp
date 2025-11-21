#include "svg-geometry-box.h"
#include "svg-resource-box.h"

#include <cmath>

namespace plutobook {

Rect SvgMarkerPosition::markerBoundingBox(float strokeWidth) const
{
    return m_marker->markerBoundingBox(m_origin, m_angle, strokeWidth);
}

void SvgMarkerPosition::renderMarker(const SvgRenderState& state, float strokeWidth) const
{
    m_marker->renderMarker(state, m_origin, m_angle, strokeWidth);
}

SvgGeometryBox::SvgGeometryBox(ClassKind type, SvgGeometryElement* element, const RefPtr<BoxStyle>& style)
    : SvgBoxModel(type, element, style)
{
}

Rect SvgGeometryBox::fillBoundingBox() const
{
    if(!m_fillBoundingBox.isValid())
        m_fillBoundingBox = path().boundingRect();
    return m_fillBoundingBox;
}

Rect SvgGeometryBox::strokeBoundingBox() const
{
    if(m_strokeBoundingBox.isValid())
        return m_strokeBoundingBox;
    m_strokeBoundingBox = fillBoundingBox();
    if(style()->hasStroke()) {
        auto strokeData = element()->getStrokeData(style());
        auto caplimit = strokeData.lineWidth() / 2.f;
        if(strokeData.lineCap() == LineCap::Square)
            caplimit *= kSqrt2;
        auto joinlimit = strokeData.lineWidth() / 2.f;
        if(strokeData.lineJoin() == LineJoin::Miter) {
            joinlimit *= strokeData.miterLimit();
        }

        m_strokeBoundingBox.inflate(std::max(caplimit, joinlimit));
    }

    if(!m_markerPositions.empty()) {
        SvgLengthContext lengthContext(element());
        auto strokeWidth = lengthContext.valueForLength(style()->strokeWidth());
        for(const auto& markerPosition : m_markerPositions) {
            m_strokeBoundingBox.unite(markerPosition.markerBoundingBox(strokeWidth));
        }
    }

    return m_strokeBoundingBox;
}

void SvgGeometryBox::render(const SvgRenderState& state) const
{
    if(style()->visibility() != Visibility::Visible)
        return;
    SvgBlendInfo blendInfo(m_clipper, m_masker, style());
    SvgRenderState newState(blendInfo, this, state, element()->transform());
    if(newState.mode() == SvgRenderMode::Clipping) {
        newState->setColor(Color::White);
        newState->fillPath(path(), style()->clipRule());
    } else {
        if(m_fill.isRenderable()) {
            m_fill.applyPaint(newState);
            newState->fillPath(path(), style()->fillRule());
        }

        if(m_stroke.isRenderable()) {
            m_stroke.applyPaint(newState);
            newState->strokePath(path(), element()->getStrokeData(style()));
        }

        if(!m_markerPositions.empty()) {
            SvgLengthContext lengthContext(element());
            auto strokeWidth = lengthContext.valueForLength(style()->strokeWidth());
            for(const auto& markerPosition : m_markerPositions) {
                markerPosition.renderMarker(newState, strokeWidth);
            }
        }
    }
}

void SvgGeometryBox::layout()
{
    m_strokeBoundingBox = Rect::Invalid;
    SvgBoxModel::layout();
    updateMarkerPositions();
}

void SvgGeometryBox::build()
{
    m_fill = element()->getPaintServer(style()->fill(), style()->fillOpacity());
    m_stroke = element()->getPaintServer(style()->stroke(), style()->strokeOpacity());
    m_markerStart = element()->getMarker(style()->markerStart());
    m_markerMid = element()->getMarker(style()->markerMid());
    m_markerEnd = element()->getMarker(style()->markerEnd());
    SvgBoxModel::build();
}

void SvgGeometryBox::updateMarkerPositions()
{
    m_markerPositions.clear();
    if(m_markerStart == nullptr && m_markerMid == nullptr && m_markerEnd == nullptr) {
        return;
    }

    Point origin;
    Point startPoint;
    Point inslopePoints[2];
    Point outslopePoints[2];

    int index = 0;
    std::array<Point, 3> points;
    PathIterator it(path());

    while(!it.isDone()) {
        switch(it.currentSegment(points)) {
        case PathCommand::MoveTo:
            startPoint = points[0];
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::LineTo:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = points[0];
            break;
        case PathCommand::CubicTo:
            inslopePoints[0] = points[1];
            inslopePoints[1] = points[2];
            origin = points[2];
            break;
        case PathCommand::Close:
            inslopePoints[0] = origin;
            inslopePoints[1] = points[0];
            origin = startPoint;
            startPoint = Point();
            break;
        }

        it.next();

        if(!it.isDone() && (m_markerStart || m_markerMid)) {
            it.currentSegment(points);
            outslopePoints[0] = origin;
            outslopePoints[1] = points[0];
            if(index == 0 && m_markerStart) {
                auto slope = outslopePoints[1] - outslopePoints[0];
                auto angle = rad2deg(std::atan2(slope.y, slope.x));
                const auto& orient = m_markerStart->element()->orient();
                if(orient.orientType() == SvgAngle::OrientType::AutoStartReverse)
                    angle -= 180.f;
                m_markerPositions.emplace_back(m_markerStart, origin, angle);
            }

            if(index > 0 && m_markerMid) {
                auto inslope = inslopePoints[1] - inslopePoints[0];
                auto outslope = outslopePoints[1] - outslopePoints[0];
                auto inangle = rad2deg(std::atan2(inslope.y, inslope.x));
                auto outangle = rad2deg(std::atan2(outslope.y, outslope.x));
                if(std::abs(inangle - outangle) > 180.f)
                    inangle += 360.f;
                auto angle = (inangle + outangle) * 0.5f;
                m_markerPositions.emplace_back(m_markerMid, origin, angle);
            }
        }

        if(m_markerEnd && it.isDone()) {
            auto slope = inslopePoints[1] - inslopePoints[0];
            auto angle = rad2deg(std::atan2(slope.y, slope.x));
            m_markerPositions.emplace_back(m_markerEnd, origin, angle);
        }

        index += 1;
    }
}

SvgPathBox::SvgPathBox(SvgPathElement* element, const RefPtr<BoxStyle>& style)
    : SvgGeometryBox(classKind, element, style)
{
}

SvgShapeBox::SvgShapeBox(SvgShapeElement* element, const RefPtr<BoxStyle>& style)
    : SvgGeometryBox(classKind, element, style)
{
}

void SvgShapeBox::layout()
{
    m_path.clear();
    m_fillBoundingBox = element()->getPath(m_path);
    SvgGeometryBox::layout();
}

} // namespace plutobook
