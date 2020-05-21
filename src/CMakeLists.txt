# check for packages
find_package(PkgConfig REQUIRED)

find_package(Threads)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)
pkg_check_modules(AIRSPY REQUIRED libairspy)


# sources
set(mlrpt_SOURCES
    common/shared.c
    decoder/bitop.c
    decoder/correlator.c
    decoder/dct.c
    decoder/ecc.c
    decoder/huffman.c
    decoder/medet.c
    decoder/met_jpg.c
    decoder/met_packet.c
    decoder/met_to_data.c
    decoder/rectify_meteor.c
    decoder/viterbi27.c
    demodulator/agc.c
    demodulator/demod.c
    demodulator/doqpsk.c
    demodulator/filters.c
    demodulator/pll.c
    mlrpt/clahe.c
    mlrpt/image.c
    mlrpt/jpeg.c
    mlrpt/main.c
    mlrpt/operation.c
    mlrpt/rc_config.c
    mlrpt/utils.c
    sdr/airspy.c
    sdr/filters.c
    sdr/rtlsdr.c)

set(mlrpt_HEADERS
    common/common.h
    common/shared.h
    decoder/bitop.h
    decoder/correlator.h
    decoder/dct.h
    decoder/ecc.h
    decoder/huffman.h
    decoder/medet.h
    decoder/met_jpg.h
    decoder/met_packet.h
    decoder/met_to_data.h
    decoder/rectify_meteor.h
    decoder/viterbi27.h
    demodulator/agc.h
    demodulator/demod.h
    demodulator/doqpsk.h
    demodulator/filters.h
    demodulator/pll.h
    mlrpt/clahe.h
    mlrpt/image.h
    mlrpt/jpeg.h
    mlrpt/operation.h
    mlrpt/rc_config.h
    mlrpt/utils.h
    sdr/airspy.h
    sdr/filters.h
    sdr/rtlsdr.h)


# primary and the only target
add_executable(mlrpt ${mlrpt_SOURCES} ${mlrpt_HEADERS})


# some preprocessor definitions
target_compile_definitions(mlrpt PRIVATE PACKAGE_NAME="${PROJECT_NAME}")
target_compile_definitions(mlrpt PRIVATE PACKAGE_STRING="${PROJECT_NAME} ${PROJECT_VERSION}")
target_compile_definitions(mlrpt PRIVATE PACKAGE_DATADIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}")

target_compile_definitions(mlrpt PRIVATE _FORTIFY_SOURCE=2)


# specific compiler flags
target_compile_options(mlrpt PRIVATE -Wall -pedantic -Werror=format-security)
target_compile_options(mlrpt PRIVATE -fstack-protector-strong)


# where our includes reside
target_include_directories(mlrpt SYSTEM PRIVATE ${AIRSPY_INCLUDE_DIRS})
target_include_directories(mlrpt SYSTEM PRIVATE ${RTLSDR_INCLUDE_DIRS})


# where to find external libraries
target_link_directories(mlrpt PRIVATE ${AIRSPY_LIBRARY_DIRS})
target_link_directories(mlrpt PRIVATE ${RTLSDR_LIBRARY_DIRS})


# link libraries
target_link_libraries(mlrpt PRIVATE m)
target_link_libraries(mlrpt PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(mlrpt PRIVATE ${AIRSPY_LIBRARIES})
target_link_libraries(mlrpt PRIVATE ${RTLSDR_LIBRARIES})


# GNU11 standard
set_target_properties(mlrpt PROPERTIES C_STANDARD 11)


# install
install(TARGETS mlrpt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
