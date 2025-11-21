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

    extern template bool is<ListItemBox>(const Box& value);

    class InsideListMarkerBox final : public InlineBox {
    public:
        static constexpr ClassKind classKind = ClassKind::InsideListMarker;

        InsideListMarkerBox(const RefPtr<BoxStyle>& style);

        const char* name() const final { return "InsideListMarkerBox"; }
    };

    extern template bool is<InsideListMarkerBox>(const Box& value);

    class OutsideListMarkerBox final : public BlockFlowBox {
    public:
        static constexpr ClassKind classKind = ClassKind::OutsideListMarker;

        OutsideListMarkerBox(const RefPtr<BoxStyle>& style);

        const char* name() const final { return "OutsideListMarkerBox"; }
    };

    extern template bool is<OutsideListMarkerBox>(const Box& value);
} // namespace plutobook