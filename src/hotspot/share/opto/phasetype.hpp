/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_VM_OPTO_PHASETYPE_HPP
#define SHARE_VM_OPTO_PHASETYPE_HPP

enum CompilerPhaseType {
  PHASE_BEFORE_STRINGOPTS,
  PHASE_AFTER_STRINGOPTS,
  PHASE_BEFORE_REMOVEUSELESS,
  PHASE_AFTER_PARSING,
  PHASE_ITER_GVN1,
  PHASE_PHASEIDEAL_BEFORE_EA,
  PHASE_ITER_GVN_AFTER_EA,
  PHASE_ITER_GVN_AFTER_ELIMINATION,
  PHASE_PHASEIDEALLOOP1,
  PHASE_PHASEIDEALLOOP2,
  PHASE_PHASEIDEALLOOP3,
  PHASE_CPP1,
  PHASE_ITER_GVN2,
  PHASE_PHASEIDEALLOOP_ITERATIONS,
  PHASE_OPTIMIZE_FINISHED,
  PHASE_GLOBAL_CODE_MOTION,
  PHASE_FINAL_CODE,
  PHASE_AFTER_EA,
  PHASE_BEFORE_CLOOPS,
  PHASE_AFTER_CLOOPS,
  PHASE_BEFORE_BEAUTIFY_LOOPS,
  PHASE_AFTER_BEAUTIFY_LOOPS,
  PHASE_BEFORE_MATCHING,
  PHASE_MATCHING,
  PHASE_INCREMENTAL_INLINE,
  PHASE_INCREMENTAL_BOXING_INLINE,
  PHASE_BARRIER_EXPANSION,
  PHASE_BEFORE_MACRO_EXPANSION,
  PHASE_END,
  PHASE_FAILURE,

  PHASE_NUM_TYPES
};

class CompilerPhaseTypeHelper {
  public:
  static const char* to_string(CompilerPhaseType cpt) {
    switch (cpt) {
      case PHASE_BEFORE_STRINGOPTS:          return "Before StringOpts";
      case PHASE_AFTER_STRINGOPTS:           return "After StringOpts";
      case PHASE_BEFORE_REMOVEUSELESS:       return "Before RemoveUseless";
      case PHASE_AFTER_PARSING:              return "After Parsing";
      case PHASE_ITER_GVN1:                  return "Iter GVN 1";
      case PHASE_PHASEIDEAL_BEFORE_EA:       return "PhaseIdealLoop before EA";
      case PHASE_ITER_GVN_AFTER_EA:          return "Iter GVN after EA";
      case PHASE_ITER_GVN_AFTER_ELIMINATION: return "Iter GVN after eliminating allocations and locks";
      case PHASE_PHASEIDEALLOOP1:            return "PhaseIdealLoop 1";
      case PHASE_PHASEIDEALLOOP2:            return "PhaseIdealLoop 2";
      case PHASE_PHASEIDEALLOOP3:            return "PhaseIdealLoop 3";
      case PHASE_CPP1:                       return "PhaseCPP 1";
      case PHASE_ITER_GVN2:                  return "Iter GVN 2";
      case PHASE_PHASEIDEALLOOP_ITERATIONS:  return "PhaseIdealLoop iterations";
      case PHASE_OPTIMIZE_FINISHED:          return "Optimize finished";
      case PHASE_GLOBAL_CODE_MOTION:         return "Global code motion";
      case PHASE_FINAL_CODE:                 return "Final Code";
      case PHASE_AFTER_EA:                   return "After Escape Analysis";
      case PHASE_BEFORE_CLOOPS:              return "Before CountedLoop";
      case PHASE_AFTER_CLOOPS:               return "After CountedLoop";
      case PHASE_BEFORE_BEAUTIFY_LOOPS:      return "Before beautify loops";
      case PHASE_AFTER_BEAUTIFY_LOOPS:       return "After beautify loops";
      case PHASE_BEFORE_MATCHING:            return "Before matching";
      case PHASE_MATCHING:                   return "After matching";
      case PHASE_INCREMENTAL_INLINE:         return "Incremental Inline";
      case PHASE_INCREMENTAL_BOXING_INLINE:  return "Incremental Boxing Inline";
      case PHASE_BARRIER_EXPANSION:          return "Barrier expand";
      case PHASE_BEFORE_MACRO_EXPANSION:     return "Before macro expansion";
      case PHASE_END:                        return "End";
      case PHASE_FAILURE:                    return "Failure";
      default:
        ShouldNotReachHere();
        return NULL;
    }
  }
};

#endif //SHARE_VM_OPTO_PHASETYPE_HPP
