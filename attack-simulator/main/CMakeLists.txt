# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c "unothorized_channel.c" "evil_twin.c" "hidden_network.c" "deauth_attack.c" "open_network.c" "menu_selection.c"      # list the source files of this component
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-zmuldefs")
