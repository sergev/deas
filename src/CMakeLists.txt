add_library(util STATIC
    lib.c
    util.c
)

#
# deas - GUI client
#
add_executable(deas
    main.cc
    help.cc
    users.cc
    accounts.cc
    journal.cc
    entries.cc
)
target_link_libraries(deas util screen pgp termcap)

#
# deasd - server
#
add_executable(deasd
    server.c
)
target_link_libraries(deasd util pgp)

#
# deasctl - management utility
#
add_executable(deasctl
    deasctl.c
)
target_link_libraries(deasctl pgp)

install(TARGETS deas deasd deasctl DESTINATION bin)
