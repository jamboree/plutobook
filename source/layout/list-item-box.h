#pragma once

#include "block-box.h"
#include "inline-box.h"

namespace plutobook {
    class ListItemBox final : public BlockFlowBox {
    public:
        static constexpr ClassKind classKind = ClassKind::ListItem;

        ListItemBox(Node* node, const RefPtr<BoxStyle>& style);

        const char* name() const final { return "ListItemBox"; }
    };

    class InsideListMarkerBox final : public InlineBox {
    public:
        static constexpr ClassKind classKind = ClassKind::InsideListMarker;

        InsideListMarkerBox(const RefPtr<BoxStyle>& style);

        const char* name() const final { return "InsideListMarkerBox"; }
    };

    class OutsideListMarkerBox final : public BlockFlowBox {
    public:
        static constexpr ClassKind classKind = ClassKind::OutsideListMarker;

        OutsideListMarkerBox(const RefPtr<BoxStyle>& style);

        const char* name() const final { return "OutsideListMarkerBox"; }
    };
} // namespace plutobook