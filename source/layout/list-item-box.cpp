#include "list-item-box.h"

namespace plutobook {

ListItemBox::ListItemBox(Node* node, const RefPtr<BoxStyle>& style)
    : BlockFlowBox(classKind, node, style)
{
}

InsideListMarkerBox::InsideListMarkerBox(const RefPtr<BoxStyle>& style)
    : InlineBox(classKind, nullptr, style)
{
}

OutsideListMarkerBox::OutsideListMarkerBox(const RefPtr<BoxStyle>& style)
    : BlockFlowBox(classKind, nullptr, style)
{
}

} // namespace plutobook
