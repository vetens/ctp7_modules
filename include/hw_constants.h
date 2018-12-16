/*! \file
 *  \brief Header containing the hardware related constants.
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef HW_CONSTANTS_H
#define HW_CONSTANTS_H

#ifndef GEM_VARIANT
#error You must define the GEM_VARIANT constant.
#endif

#include <stdint.h>
#include <array>

/*! \brief This namespace hold the constants related to the OptoHybrid.
 */
namespace oh{
    /*! \brief GE1/1 specific namespace.
     */
    namespace ge11{
        /*! \brief The number of VFAT's per OptoHybrid.
         */
        constexpr uint32_t vfatsPerOH = 24;
    }

    using namespace GEM_VARIANT;
}

/*! \brief This namespace hold the constants related to the GBT.
 */
namespace gbt{
    /*! \brief The size of the GBT configuration address space. The corresponding addresses span from 0 to 365.
     */
    constexpr uint16_t configSize = 366;

    /*! \brief This type defines a GBT configuration blob.
     */
    typedef std::array<uint8_t, configSize> config_t;

    /*! \brief Minimal phase for the elink RX GBT phase.
     */
    constexpr uint8_t phaseMin = 0;

    /*! \brief Maximal phase for the elink RX GBT phase.
     */
    constexpr uint8_t phaseMax = 15;

    /*! \brief GE1/1 specific namespace.
     */
    namespace ge11{
        /*! \brief The number of GBT's per OptoHybrid.
         */
        constexpr uint32_t gbtsPerOH = 3;
    }

    using namespace GEM_VARIANT;

    /*! \brief Mappings between elinks, GBT index and VFAT index.
     */
    namespace elinkMappings {
        namespace ge11{
            /*! \brief Mapping from VFAT index to GBT index.
             */
            constexpr std::array<uint32_t, 24> vfatToGBT{
                { 1, 1, 1, 1, 1, 1, 1, 0, 1, 2, 2, 2, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 0 }
            };

            /*! \brief Mapping from VFAT index to the elink of its corresponding GBT.
             */
            constexpr std::array<uint8_t, 24> vfatToElink{
                { 5, 9, 2, 3, 1, 8, 6, 6, 4, 1, 5, 4, 3, 2, 1, 0, 7, 8, 6, 7, 2, 3, 9, 8 }
            };

            /*! \brief Mapping from elink index to the 3 registers addresses in the GBT.
	         */
            constexpr std::array<std::array<uint16_t, 3>, 10> elinkToRegisters{
                { {69, 73, 77}, {67, 71, 75}, {93, 97, 101}, {91, 95, 99}, {117, 121, 125}, {115, 119, 123}, {141, 145, 149}, {139, 143, 147}, {165, 169, 173}, {163, 167, 171} }
            };
        }

        using namespace GEM_VARIANT;
    }
}

#endif

