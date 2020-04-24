/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#ifndef ILXQTPANELPLUGIN_H
#define ILXQTPANELPLUGIN_H

#include <QtPlugin>
#include "ilxqtpanel.h"
#include "lxqtpanelglobals.h"

/**
LXQt panel plugins are standalone sharedlibraries
(*.so) located in PLUGIN_DIR (define provided by CMakeLists.txt).

Plugin for the panel is a library written in C++. One more necessary thing
is a .desktop file describing this plugin. The same may be additional files,
like translations. Themselves plugins will be installed to
/usr/local/lib/lxqt-panel or /usr/lib/lxqt-panel (dependent on cmake option
-DCMAKE_INSTALL_PREFIX). Desktop files are installed to
/usr/local/share/lxqt/lxqt-panel, translations to
/usr/local/share/lxqt/lxqt-panel/PLUGIN_NAME.
**/

class QDialog;

/** \brief Base abstract class for LXQt panel widgets/plugins.
All plugins *must* be inherited from this one.

This class provides some basic API and inherited/implemented
plugins GUIs will be responsible on the functionality itself.
**/

class LXQT_PANEL_API ILXQtPanelPlugin
{
public:
    /**
      This enum describes the reason the plugin was activated.
     **/
    enum ActivationReason {
        Unknown             = 0,    ///< Unknown reason
        DoubleClick         = 2,    ///<	The plugin entry was double clicked
        Trigger             = 3,    ///<	The plugin was clicked
        MiddleClick         = 4     ///< The plugin was clicked with the middle mouse button
    };

    /**
     Constructs an ILXQtPanelPlugin object with the given startupInfo. You do not have to worry
     about the startupInfo parameters, ILXQtPanelPlugin processes the parameters itself.
     **/
    ILXQtPanelPlugin(ILXQtPanel *lxqtPanel):
        mPanel(lxqtPanel) {}

    /**
     Destroys the object.
     **/
    virtual ~ILXQtPanelPlugin() {}

    /**
     From the user's point of view, your plugin is some visual widget on the panel. This function returns a pointer to it.
     This method is called only once, so you are free to return the pointer on a class member, or create the widget on the fly.
     **/
    virtual QWidget *widget() = 0;

    /**
    Returns the plugin settings dialog. Reimplement this function if your plugin has it.
    The panel does not take ownership of the dialog, it is probably a good idea to set Qt::WA_DeleteOnClose
    attribute for the dialog.
    The default implementation returns 0, no dialog;

    Note that the flags method has to return HaveConfigDialog flag.
    To save the settings you should use a ready-to-use ILXQtPanelPlugin::settings() object.

    **/
    virtual QDialog *configureDialog() { return 0; }

    /**
    This function is called when values are changed in the plugin settings.
    Reimplement this function to your plugin corresponded the new settings.

    The default implementation do nothing.
    **/
    virtual void settingsChanged() {}

    /**
    This function is called when the user activates the plugin. reason specifies the reason for activation.
    ILXQtPanelPlugin::ActivationReason enumerates the various reasons.

    The default implementation do nothing.
     **/
    virtual void activated(ActivationReason reason) {}

    /**
    This function is called when the panel geometry or lines count are changed.

    The default implementation do nothing.

     **/
    virtual void realign() {}

    /**
    Returns the panel object.
     **/
    ILXQtPanel *panel() const { return mPanel; }

    /**
     Helper functions for calculating global screen position of some popup window with windowSize size.
     If you need to show some popup window, you can use it, to get global screen position for the new window.
     **/
    virtual QRect calculatePopupWindowPos(const QSize &windowSize)
    {
        return mPanel->calculatePopupWindowPos(this, windowSize);
    }

    /*!
     * \brief By calling this function plugin notifies the panel about showing a (standalone) window/menu.
     *
     * \param w the shown window
     *
     */
    inline void willShowWindow(QWidget * w)
    {
        mPanel->willShowWindow(w);
    }

    /*!
     * \brief By calling this function, a plugin notifies the panel about change of it's "static"
     * configuration
     *
     * \sa isSeparate(), isExpandable
     */
    inline void pluginFlagsChanged()
    {
        mPanel->pluginFlagsChanged(this);
    }

    virtual bool isSeparate() const { return false;  }
    virtual bool isExpandable() const { return false; }
private:
    ILXQtPanel *mPanel;
};

#endif // ILXQTPANELPLUGIN_H
