/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */


#ifndef LXQTPANELAPPLICATION_H
#define LXQTPANELAPPLICATION_H

#include <LXQt/Application>
#include "lxqtpanel.h"

/*!
 * \brief The LXQtPanelApplication class inherits from LXQt::Application and
 * is therefore the QApplication that we will create and execute in our
 * main()-function.
 *
 * LXQtPanelApplication itself is not a visible panel, rather it is only
 * the container which holds the visible panels. These visible panels are
 * LXQtPanel objects which are stored in mPanels. This approach enables us
 * to have more than one panel (for example one panel at the top and one
 * panel at the bottom of the screen) without additional effort.
 */
class LXQtPanelApplication : public LXQt::Application
{
    Q_OBJECT
public:
    /*!
     * \brief Creates a new LXQtPanelApplication with the given command line
     * arguments. Performs the following steps:
     * 1. Initializes the LXQt::Application, sets application name and version.
     * 2. Handles command line arguments. Currently, the only cmdline argument
     * is -c = -config = -configfile which chooses a different config file
     * for the LXQt::Settings.
     * 3. Creates the LXQt::Settings.
     * 4. Connects QCoreApplication::aboutToQuit to cleanup().
     * 5. Calls addPanel() for each panel found in the config file. If there is
     * none, adds a new panel.
     * \param argc
     * \param argv
     */
    explicit LXQtPanelApplication(int& argc, char** argv);
    ~LXQtPanelApplication();

private:
    QPointer<LXQtPanel> mPanel;

    Q_DISABLE_COPY(LXQtPanelApplication)
};


#endif // LXQTPANELAPPLICATION_H
