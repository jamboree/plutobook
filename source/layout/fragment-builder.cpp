#include "fragment-builder.h"

namespace plutobook {

float FragmentBuilder::applyFragmentBreakBefore(const BoxFrame* child, float offset)
{
    if(!needsBreakBetween(child->style()->breakBefore()))
        return offset;
    auto fragmentHeight = fragmentHeightForOffset(offset);
    addForcedFragmentBreak(offset);
    if(fragmentHeight > 0.f)
        offset += fragmentRemainingHeightForOffset(offset, AssociateWithFormerFragment);
    return offset;
}

float FragmentBuilder::applyFragmentBreakAfter(const BoxFrame* child, float offset)
{
    if(!needsBreakBetween(child->style()->breakAfter()))
        return offset;
    auto fragmentHeight = fragmentHeightForOffset(offset);
    addForcedFragmentBreak(offset);
    if(fragmentHeight > 0.f)
        offset += fragmentRemainingHeightForOffset(offset, AssociateWithFormerFragment);
    return offset;
}

float FragmentBuilder::applyFragmentBreakInside(const BoxFrame* child, float offset)
{
    if(!child->isReplaced() && !needsBreakInside(child->style()->breakInside()))
        return offset;
    auto childHeight = child->height();
    if(child->isFloating())
        childHeight += child->marginHeight();
    auto fragmentHeight = fragmentHeightForOffset(offset);
    updateMinimumFragmentHeight(offset, childHeight);
    if(fragmentHeight == 0.f)
        return offset;
    auto remainingHeight = fragmentRemainingHeightForOffset(offset, AssociateWithLatterFragment);
    if(remainingHeight < childHeight && remainingHeight < fragmentHeight)
        return offset + remainingHeight;
    return offset;
}

bool FragmentBuilder::needsBreakBetween(BreakBetween between) const
{
    if(fragmentType() == FragmentType::Column)
        return between == BreakBetween::Column;
    return between >= BreakBetween::Page;
}

bool FragmentBuilder::needsBreakInside(BreakInside inside) const
{
    if(fragmentType() == FragmentType::Page)
        return inside == BreakInside::Avoid || inside == BreakInside::AvoidPage;
    return inside == BreakInside::Avoid || inside == BreakInside::AvoidColumn;
}

} // namespace plutobook