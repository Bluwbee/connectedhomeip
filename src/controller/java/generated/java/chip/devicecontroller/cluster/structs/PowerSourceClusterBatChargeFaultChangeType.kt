/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package chip.devicecontroller.cluster.structs

import chip.devicecontroller.cluster.*
import chip.tlv.AnonymousTag
import chip.tlv.ContextSpecificTag
import chip.tlv.Tag
import chip.tlv.TlvReader
import chip.tlv.TlvWriter

class PowerSourceClusterBatChargeFaultChangeType(
  val current: List<UInt>,
  val previous: List<UInt>
) {
  override fun toString(): String = buildString {
    append("PowerSourceClusterBatChargeFaultChangeType {\n")
    append("\tcurrent : $current\n")
    append("\tprevious : $previous\n")
    append("}\n")
  }

  fun toTlv(tlvTag: Tag, tlvWriter: TlvWriter) {
    tlvWriter.apply {
      startStructure(tlvTag)
      startArray(ContextSpecificTag(TAG_CURRENT))
      for (item in current.iterator()) {
        put(AnonymousTag, item)
      }
      endArray()
      startArray(ContextSpecificTag(TAG_PREVIOUS))
      for (item in previous.iterator()) {
        put(AnonymousTag, item)
      }
      endArray()
      endStructure()
    }
  }

  companion object {
    private const val TAG_CURRENT = 0
    private const val TAG_PREVIOUS = 1

    fun fromTlv(tlvTag: Tag, tlvReader: TlvReader): PowerSourceClusterBatChargeFaultChangeType {
      tlvReader.enterStructure(tlvTag)
      val current =
        buildList<UInt> {
          tlvReader.enterArray(ContextSpecificTag(TAG_CURRENT))
          while (!tlvReader.isEndOfContainer()) {
            add(tlvReader.getUInt(AnonymousTag))
          }
          tlvReader.exitContainer()
        }
      val previous =
        buildList<UInt> {
          tlvReader.enterArray(ContextSpecificTag(TAG_PREVIOUS))
          while (!tlvReader.isEndOfContainer()) {
            add(tlvReader.getUInt(AnonymousTag))
          }
          tlvReader.exitContainer()
        }

      tlvReader.exitContainer()

      return PowerSourceClusterBatChargeFaultChangeType(current, previous)
    }
  }
}
