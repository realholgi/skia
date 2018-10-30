/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrSkSLFP_DEFINED
#define GrSkSLFP_DEFINED

#include "GrCaps.h"
#include "GrFragmentProcessor.h"
#include "GrCoordTransform.h"
#include "GrShaderCaps.h"
#include "SkSLCompiler.h"
#include "SkSLPipelineStageCodeGenerator.h"
#include "SkRefCnt.h"
#include "../private/GrSkSLFPFactoryCache.h"

#if GR_TEST_UTILS
#define GR_FP_SRC_STRING const char*
#else
#define GR_FP_SRC_STRING static const char*
#endif

class GrContext;
class GrSkSLFPFactory;

class GrSkSLFP : public GrFragmentProcessor {
public:
    /**
     * Returns a new unique identifier. Each different SkSL fragment processor should call
     * NewIndex once, statically, and use this index for all calls to Make.
     */
    static int NewIndex() {
        static int index = 0;
        return sk_atomic_inc(&index);
    }

    /**
     * Creates a new fragment processor from an SkSL source string and a struct of inputs to the
     * program. The input struct's type is derived from the 'in' variables in the SkSL source, so
     * e.g. the shader:
     *
     *    in bool dither;
     *    in float x;
     *    in float y;
     *    ....
     *
     * would expect a pointer to a struct set up like:
     *
     * struct {
     *     bool dither;
     *     float x;
     *     float y;
     * };
     *
     * As turning SkSL into GLSL / SPIR-V / etc. is fairly expensive, and the output may differ
     * based on the inputs, internally the process is divided into two steps: we first parse and
     * semantically analyze the SkSL into an internal representation, and then "specialize" this
     * internal representation based on the inputs. The unspecialized internal representation of
     * the program is cached, so further specializations of the same code are much faster than the
     * first call.
     *
     * This caching is based on the 'index' parameter, which should be derived by statically calling
     * 'NewIndex()'. Each given SkSL string should have a single, statically defined index
     * associated with it.
     */
    static std::unique_ptr<GrSkSLFP> Make(
                   GrContext* context,
                   int index,
                   const char* name,
                   const char* sksl,
                   const void* inputs,
                   size_t inputSize);

    const char* name() const override;

    void addChild(std::unique_ptr<GrFragmentProcessor> child);

    std::unique_ptr<GrFragmentProcessor> clone() const override;

private:
    GrSkSLFP(sk_sp<GrSkSLFPFactoryCache> factoryCache, const GrShaderCaps* shaderCaps, int fIndex,
             const char* name, const char* sksl, const void* inputs, size_t inputSize);

    GrSkSLFP(const GrSkSLFP& other);

    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;

    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;

    bool onIsEqual(const GrFragmentProcessor&) const override;

    void createFactory() const;

    sk_sp<GrSkSLFPFactoryCache> fFactoryCache;

    const sk_sp<GrShaderCaps> fShaderCaps;

    mutable sk_sp<GrSkSLFPFactory> fFactory;

    int fIndex;

    const char* fName;

    const char* fSkSL;

    const std::unique_ptr<int8_t[]> fInputs;

    size_t fInputSize;

    mutable SkSL::String fKey;

    GR_DECLARE_FRAGMENT_PROCESSOR_TEST

    typedef GrFragmentProcessor INHERITED;

    friend class GrGLSLSkSLFP;

    friend class GrSkSLFPFactory;
};

/**
 * Produces GrFragmentProcessors from SkSL code. As the shader code produced from the SkSL depends
 * upon the inputs to the SkSL (static if's, etc.) we first create a factory for a given SkSL
 * string, then use that to create the actual GrFragmentProcessor.
 */
class GrSkSLFPFactory : public SkRefCnt {
public:
    /**
     * Constructs a GrSkSLFPFactory for a given SkSL source string. Creating a factory will
     * preprocess the SkSL and determine which of its inputs are declared "key" (meaning they cause
     * the produced shaders to differ), so it is important to reuse the same factory instance for
     * the same shader in order to avoid repeatedly re-parsing the SkSL.
     */
    GrSkSLFPFactory(const char* name, const GrShaderCaps* shaderCaps, const char* sksl);

    const SkSL::Program* getSpecialization(const SkSL::String& key, const void* inputs,
                                           size_t inputSize);

    const char* fName;

    SkSL::Compiler fCompiler;

    std::shared_ptr<SkSL::Program> fBaseProgram;

    std::vector<const SkSL::Variable*> fInputVars;

    std::vector<const SkSL::Variable*> fKeyVars;

    std::unordered_map<SkSL::String, std::unique_ptr<const SkSL::Program>> fSpecializations;

    friend class GrSkSLFP;
};

#endif
