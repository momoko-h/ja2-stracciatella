file(GLOB LOCAL_JA2_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
set(LOCAL_JA2_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/MercProfile.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MercProfileInfo.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MERCListingModel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/RPCSmallFaceModel.cc
        )

set_property(
        SOURCE ${LOCAL_JA2_SOURCES}
        DIRECTORY ${CMAKE_SOURCE_DIR}
        PROPERTY UNITY_GROUP "externalized_subdirectories"
)
            
set(JA2_SOURCES
        ${JA2_SOURCES}
        ${LOCAL_JA2_HEADERS}
        ${LOCAL_JA2_SOURCES}
        PARENT_SCOPE
        )
set(JA2_INCLUDES
        ${JA2_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}
        PARENT_SCOPE
        )
