/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tensorflow/lite/delegates/flex/allowlisted_flex_ops.h"

#include <set>

#include "tensorflow/core/framework/op.h"
#include "tensorflow/lite/delegates/flex/allowlisted_flex_ops_internal.h"

namespace tflite {
namespace flex {

const std::set<std::string>& GetFlexAllowlist() {
  // LINT.IfChange
  static const std::set<std::string>* allowlisted_flex_ops =
      new std::set<std::string>({
          // go/keep-sorted start
            "RFFT",
           "IRFFT",
           "Mul",
           "Pad",
           "StidedSlice",
           "Transpose",
           "CimplexAbs",
          // go/keep-sorted end
      });
  // LINT.ThenChange(//tensorflow/lite/g3doc/guide/op_select_allowlist.md)

  return *allowlisted_flex_ops;
  // Prevent lint error about this function being too long. This function
  // is a set of ops, and making it shorter won't help readbility.
  // NOLINTNEXTLINE
}

const std::set<std::string>& GetTFTextFlexAllowlist() {
  // LINT.IfChange
  static const std::set<std::string>* tftext_flex_ops =
      new std::set<std::string>({
          "CaseFoldUTF8",
          "ConstrainedSequence",
          "MaxSpanningTree",
          "NormalizeUTF8",
          "NormalizeUTF8WithOffsetsMap",
          "RegexSplitWithOffsets",
          "RougeL",
          "SentenceFragments",
          "SentencepieceOp",
          "SentencepieceTokenizeOp",
          "SentencepieceTokenizeWithOffsetsOp",
          "SentencepieceDetokenizeOp",
          "SentencepieceVocabSizeOp",
          "SplitMergeTokenizeWithOffsets",
          "TokenizerFromLogits",
          "UnicodeScriptTokenizeWithOffsets",
          "WhitespaceTokenizeWithOffsets",
          "WordpieceTokenizeWithOffsets",
      });
  // LINT.ThenChange(//tensorflow/lite/g3doc/guide/op_select_allowlist.md)

  return *tftext_flex_ops;
}

// Allow the tf.text ops if they are registered in the global op registry.
bool IsAllowedTFTextOpForFlex(const std::string& op_name) {
  if (GetTFTextFlexAllowlist().count(op_name) == 0) return false;
  return tensorflow::OpRegistry::Global()->LookUp(op_name) != nullptr;
}

const std::set<std::string>& GetSentencePieceFlexAllowlist() {
  // LINT.IfChange
  static const std::set<std::string>* sentencepiece_flex_ops =
      new std::set<std::string>({
          "SentencepieceGetPieceSize",
          "SentencepiecePieceToId",
          "SentencepieceIdToPiece",
          "SentencepieceEncodeDense",
          "SentencepieceEncodeSparse",
          "SentencepieceDecode",
      });
  // LINT.ThenChange(//tensorflow/lite/g3doc/guide/op_select_allowlist.md)

  return *sentencepiece_flex_ops;
}

// Allow the sentencepiece ops if they are registered in the global op registry.
bool IsAllowedSentencePieceOpForFlex(const std::string& op_name) {
  if (GetSentencePieceFlexAllowlist().count(op_name) == 0) return false;
  return tensorflow::OpRegistry::Global()->LookUp(op_name) != nullptr;
}

bool IsAllowlistedFlexOp(const std::string& tensorflow_op_name) {
  if (GetFlexAllowlist().count(tensorflow_op_name) != 0) return true;

  // Check if the op is an allowlisted tf.text or sentencepiece op.
  return IsAllowedTFTextOpForFlex(tensorflow_op_name) ||
         IsAllowedSentencePieceOpForFlex(tensorflow_op_name);
}

}  // namespace flex
}  // namespace tflite
