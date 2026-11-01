#pragma once
#include "PassElement.hpp"
#include "../types.hpp"

class CRendererHintsPassElement : public IPassElement {
  public:
    struct SData {
        std::optional<Render::SRenderModifData> renderModif;
    };

    CRendererHintsPassElement(const SData& data);
    virtual ~CRendererHintsPassElement() = default;

    virtual bool        needsLiveBlur();
    virtual bool        needsPrecomputeBlur();
    virtual bool        undiscardable();
    // bounding boxes and opaque regions of the elements are unscaled, so occlusion culling can't be trusted while a modifier is active
    virtual bool        disableSimplification();

    virtual const char* passName() {
        return "CRendererHintsPassElement";
    }

    virtual ePassElementType type() {
        return EK_HINTS;
    };

    SData m_data;
};