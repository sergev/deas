add_library(screen STATIC
    Screen.cc
    Tty.cc
    Key.cc
    Box.cc
    Popup.cc
    Dialog.cc
    Menu.cc
    Termlib.cc
    Termgoto.cc
    curses.cc
)

add_executable(hv
    hv.cc
)
target_link_libraries(hv screen termcap)

add_executable(hellcurs
    hellcurs.c
)
target_link_libraries(hellcurs screen termcap)
