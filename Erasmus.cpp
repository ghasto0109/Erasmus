#include "MainWindow.h"

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);

    HWND console = ::GetConsoleWindow();
    ::SetWindowPos(console, NULL, 20, 20, 0, 0, SWP_NOSIZE);

    MainWindow mainWindow;
    mainWindow.move(200, 40);
    mainWindow.show();

	return application.exec();
}