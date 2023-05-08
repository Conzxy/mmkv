function (mmkv_gen_proto_code)
  cmake_parse_arguments(PARSE_ARGV 0 CONZXY "" "PROTOC_PATH" "FILES")
  foreach (proto_file_fullpath ${CONZXY_FILES})
    get_filename_component(proto_filename ${proto_file_fullpath} NAME_WE)
    get_filename_component(proto_dir ${proto_file_fullpath} DIRECTORY)
    message(STATUS "Generate the pb files of ${proto_file_fullpath}")
    message(STATUS "proto_filename = ${proto_filename}")
    message(STATUS "proto_dir = ${proto_dir}")
    add_custom_command(
      OUTPUT ${proto_filename}.pb.cc ${proto_filename}.pb.h
      COMMAND protoc
      ARGS --cpp_out . ${proto_file_fullpath} -I${proto_dir}
      DEPENDS ${proto_file_fullpath}
      VERBATIM
    )
  endforeach ()
endfunction ()
