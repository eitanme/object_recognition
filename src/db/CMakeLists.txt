find_package(Boost COMPONENTS
  system
  serialization
  thread
  filesystem
  regex
  REQUIRED
  )

add_subdirectory(json_spirit)

find_package(CURL REQUIRED)
find_package(OpenCV)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../tod
  )

add_library(object_recognition_db SHARED
  couch.cpp
  db.cpp
  db_couch.cpp
  serialization_opencv.cpp
  opencv.cpp
  utils.cpp
  )

target_link_libraries(object_recognition_db
  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
  ${OpenCV_LIBS}
  json_spirit
  )

#install the main shared lib
install(TARGETS object_recognition_db
  DESTINATION lib
  COMPONENT main
  )

ectomodule(object_recognition_db
    module.cpp
    )
link_ecto(object_recognition_db
  object_recognition_db
  )