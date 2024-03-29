/*  Mpcgui for SMPlayer.
    Copyright (C) 2008 matt_ <matt@endboss.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "mpcgui.h"
#include "mpcstyles.h"
#include "widgetactions.h"
#include "autohidewidget.h"
#include "myaction.h"
#include "mplayerwindow.h"
#include "global.h"
#include "helper.h"
#include "desktopinfo.h"
#include "colorutils.h"

#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QLayout>
#include <QApplication>

using namespace Global;


MpcGui::MpcGui( QWidget * parent, Qt::WindowFlags flags )
	: BaseGuiPlus( parent, flags )
{
	createActions();
	createControlWidget();
	createStatusBar();
	createFloatingControl();
	populateMainMenu();

	retranslateStrings();

	loadConfig();

	if (pref->compact_mode) {
		controlwidget->hide();
		timeslidewidget->hide();
	}

	applyStyles();
}

MpcGui::~MpcGui() {
	saveConfig();
}

void MpcGui::createActions() {
	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();
    timeslider_action->setCustomStyle( new MpcTimeSlideStyle() );

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();
	volumeslider_action->setCustomStyle( new MpcVolumeSlideStyle() );
	volumeslider_action->setFixedSize( QSize(50,18) );
	volumeslider_action->setTickPosition( QSlider::NoTicks );
#endif
}


void MpcGui::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);

	controlwidget->setMovable(false);
	controlwidget->setAllowedAreas(Qt::BottomToolBarArea);
	controlwidget->addAction(playAct);
	controlwidget->addAction(pauseAct);
	controlwidget->addAction(stopAct);
	controlwidget->addSeparator();
	controlwidget->addAction(rewind3Act);
	controlwidget->addAction(rewind1Act);
	controlwidget->addAction(forward1Act);
	controlwidget->addAction(forward3Act);
	controlwidget->addSeparator();
	controlwidget->addAction(frameStepAct);
	controlwidget->addSeparator();

	QLabel* pLabel = new QLabel(this);
	pLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	controlwidget->addWidget(pLabel);

	controlwidget->addAction(muteAct);
	controlwidget->addAction(volumeslider_action);

	timeslidewidget = new QToolBar( this );
	timeslidewidget->setObjectName("timeslidewidget");
	timeslidewidget->setLayoutDirection(Qt::LeftToRight);
	timeslidewidget->addAction(timeslider_action);
	timeslidewidget->setMovable(false);

	/*
	QColor SliderColor = palette().color(QPalette::Window);
	QColor SliderBorderColor = palette().color(QPalette::Dark);
	*/
	setIconSize( QSize( 16 , 16 ) );

	addToolBar(Qt::BottomToolBarArea, controlwidget);
	addToolBarBreak(Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, timeslidewidget);

	controlwidget->setStyle(new MpcToolbarStyle() );
	timeslidewidget->setStyle(new MpcToolbarStyle() );

	statusBar()->show();
}

void MpcGui::createFloatingControl() {
	// Floating control
	floating_control = new AutohideWidget(mplayerwindow);
	floating_control->setAutoHide(true);
	floating_control->hide();
	spacer = new QSpacerItem(10,10);

	floating_control_time = new QLabel(floating_control);
	floating_control_time->setAlignment(Qt::AlignRight);
	floating_control_time->setAutoFillBackground(true);
	floating_control_time->setObjectName("floating_control_time");
#ifdef CHANGE_WIDGET_COLOR
	ColorUtils::setBackgroundColor( floating_control_time, QColor(0,0,0) );
	ColorUtils::setForegroundColor( floating_control_time, QColor(255,255,255) );
#endif

	connect(this, SIGNAL(timeChanged(QString)), floating_control_time, SLOT(setText(QString)));
}

void MpcGui::retranslateStrings() {
	BaseGuiPlus::retranslateStrings();

	controlwidget->setWindowTitle( tr("Control bar") );
	timeslidewidget->setWindowTitle( tr("Seek bar") );

	setupIcons();
}

#if AUTODISABLE_ACTIONS
void MpcGui::enableActionsOnPlaying() {
	BaseGuiPlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void MpcGui::disableActionsOnStop() {
	BaseGuiPlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}
#endif // AUTODISABLE_ACTIONS

void MpcGui::aboutToEnterFullscreen() {
	BaseGuiPlus::aboutToEnterFullscreen();

	// Show floating_control
	// Move controls to the floating_control layout
	removeToolBarBreak(controlwidget);
	removeToolBar(controlwidget);
	removeToolBar(timeslidewidget);
	floating_control->layout()->addWidget(timeslidewidget);
	floating_control->layout()->addItem(spacer);
	floating_control->layout()->addWidget(controlwidget);
	floating_control->layout()->addWidget(floating_control_time);
	controlwidget->show();
	timeslidewidget->show();
	floating_control->adjustSize();

	floating_control->setMargin(pref->floating_control_margin);
	floating_control->setPercWidth(pref->floating_control_width);
	floating_control->setAnimated(pref->floating_control_animated);
	floating_control->setActivationArea( (AutohideWidget::Activation) pref->floating_activation_area);
	floating_control->setHideDelay(pref->floating_hide_delay);
	QTimer::singleShot(100, floating_control, SLOT(activate()));


	if (!pref->compact_mode) {
		//controlwidget->hide();
		//timeslidewidget->hide();
		statusBar()->hide();
	}
}

void MpcGui::aboutToExitFullscreen() {
	BaseGuiPlus::aboutToExitFullscreen();

	// Remove controls from the floating_control and put them back to the mainwindow
	floating_control->deactivate();
	floating_control->layout()->removeWidget(controlwidget);
	floating_control->layout()->removeWidget(timeslidewidget);
	floating_control->layout()->removeItem(spacer);
	floating_control->layout()->removeWidget(floating_control_time);
	addToolBar(Qt::BottomToolBarArea, controlwidget);
	addToolBarBreak(Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, timeslidewidget);

	if (!pref->compact_mode) {
		controlwidget->show();
		statusBar()->show();
		timeslidewidget->show();
	} else {
		controlwidget->hide();
		timeslidewidget->hide();
	}
}

void MpcGui::aboutToEnterCompactMode() {
	BaseGuiPlus::aboutToEnterCompactMode();

	controlwidget->hide();
	timeslidewidget->hide();
	statusBar()->hide();
}

void MpcGui::aboutToExitCompactMode() {
	BaseGuiPlus::aboutToExitCompactMode();

	statusBar()->show();
	controlwidget->show();
	timeslidewidget->show();
}

#if USE_mpcMUMSIZE
QSize MpcGui::mpcmumSizeHint() const {
	return QSize(controlwidget->sizeHint().width(), 0);
}
#endif


void MpcGui::saveConfig() {
	QSettings * set = settings;

	set->beginGroup( "mpc_gui");

	if (pref->save_window_size_on_exit) {
		qDebug("MpcGui::saveConfig: w: %d h: %d", width(), height());
		set->setValue( "pos", pos() );
		set->setValue( "size", size() );
		set->setValue( "state", (int) windowState() );
	}

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

	set->endGroup();
}

void MpcGui::loadConfig() {
	QSettings * set = settings;

	set->beginGroup( "mpc_gui");

	if (pref->save_window_size_on_exit) {
		QPoint p = set->value("pos", pos()).toPoint();
		QSize s = set->value("size", size()).toSize();

		if ( (s.height() < 200) && (!pref->use_mplayer_window) ) {
			s = pref->default_size;
		}

		move(p);
		resize(s);

		setWindowState( (Qt::WindowStates) set->value("state", 0).toInt() );

		if (!DesktopInfo::isInsideScreen(this)) {
			QPoint tl = DesktopInfo::topLeftPrimaryScreen();
			move(tl);
			qWarning("DefaultGui::loadConfig: window is outside of the screen, moved to %d x %d", tl.x(), tl.y());
		}
	}

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	set->endGroup();
}

void MpcGui::setupIcons() {
    playAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(0,0,16,16) );
    playOrPauseAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(0,0,16,16) );
    pauseAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(16,0,16,16) );
    pauseAndStepAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(16,0,16,16) );
    stopAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(32,0,16,16) );
    rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );
    rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
    rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
    forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
    forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
    forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );
    frameStepAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(144,0,16,16) );
    muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(192,0,16,16) );

    pauseAct->setCheckable(true);
    playAct->setCheckable(true);
    stopAct->setCheckable(true);

	connect( muteAct, SIGNAL(toggled(bool)),
             this, SLOT(muteIconChange(bool)) );

	connect( core , SIGNAL(mediaInfoChanged()),
             this, SLOT(updateAudioChannels()) );

	connect( core , SIGNAL(stateChanged(Core::State)),
             this, SLOT(iconChange(Core::State)) );
}

void MpcGui::iconChange(Core::State state) {
    playAct->blockSignals(true);
    pauseAct->blockSignals(true);
    stopAct->blockSignals(true);

    if( state == Core::Paused )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(true);
        stopAct->setChecked(false);
    }
    if( state == Core::Playing )
    {
        playAct->setChecked(true);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }
    if( state == Core::Stopped )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }

    playAct->blockSignals(false);
    pauseAct->blockSignals(false);
    stopAct->blockSignals(false);
}

void MpcGui::muteIconChange(bool b) {
    if( sender() == muteAct )
    {
        if(!b) {
            muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(192,0,16,16) );
        } else {
            muteAct->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(208,0,16,16) );
        }
    }

}


void MpcGui::createStatusBar() {
    // remove frames around statusbar items
    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");

    // emulate mono/stereo display from mpc
    audiochannel_display = new QLabel( statusBar() );
    audiochannel_display->setContentsMargins(0,0,0,0);
    audiochannel_display->setAlignment(Qt::AlignRight);
    audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_stereo.png") );
    audiochannel_display->setMinimumSize(audiochannel_display->sizeHint());
    audiochannel_display->setMaximumSize(audiochannel_display->sizeHint());
    audiochannel_display->setPixmap( QPixmap("") );

	time_display = new QLabel( statusBar() );
	time_display->setAlignment(Qt::AlignRight);
	time_display->setText(" 88:88:88 / 88:88:88 ");
	time_display->setMinimumSize(time_display->sizeHint());
	time_display->setContentsMargins(15,2,1,1);

	frame_display = new QLabel( statusBar() );
	frame_display->setAlignment(Qt::AlignRight);
	frame_display->setText("88888888");
	frame_display->setMinimumSize(frame_display->sizeHint());
	frame_display->setContentsMargins(15,2,1,1);

	statusBar()->setAutoFillBackground(true);

#ifdef CHANGE_WIDGET_COLOR
	ColorUtils::setBackgroundColor( statusBar(), QColor(0,0,0) );
	ColorUtils::setForegroundColor( statusBar(), QColor(255,255,255) );
	ColorUtils::setBackgroundColor( time_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( time_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( frame_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( frame_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( audiochannel_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( audiochannel_display, QColor(255,255,255) );
#endif

	statusBar()->setSizeGripEnabled(false);

	statusBar()->addPermanentWidget( frame_display, 0 );
	frame_display->setText( "0" );

	statusBar()->addPermanentWidget( time_display, 0 );
	time_display->setText(" 00:00:00 / 00:00:00 ");

	statusBar()->addPermanentWidget( audiochannel_display, 0 );

	time_display->show();
	frame_display->hide();

	connect(this, SIGNAL(timeChanged(QString)), time_display, SLOT(setText(QString)));
	connect(this, SIGNAL(frameChanged(int)), this, SLOT(displayFrame(int)));
}

void MpcGui::displayFrame(int frame) {
	if (frame_display->isVisible()) {
		frame_display->setNum( frame );
	}
}

void MpcGui::updateAudioChannels() {
    if( core->mdat.audio_nch == 1 ) {
        audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_mono.png") );
    }
    else {
        audiochannel_display->setPixmap( QPixmap(":/mpcgui/mpc_stereo.png") );
    }
}

void MpcGui::showFullscreenControls() {

    if(pref->fullscreen && controlwidget->isHidden() && timeslidewidget->isHidden() && 
        !pref->compact_mode )
    {
        controlwidget->show();
        timeslidewidget->show();
        statusBar()->show();
    }
}

void MpcGui::hideFullscreenControls() {

    if(pref->fullscreen && controlwidget->isVisible() && timeslidewidget->isVisible() )
    {
        controlwidget->hide();
        timeslidewidget->hide();
        statusBar()->hide();
    }
}

void MpcGui::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	/*
	if (qApp->isLeftToRight()) {
	*/
        rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );

        forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );
	/*
	} else {
        rewind1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(96,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(112,0,16,16) );

        forward1Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        forward2Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(80,0,16,16) );
        forward3Act->setIcon( QPixmap(":/mpcgui/mpc_toolbar.png").copy(64,0,16,16) );
	}
	*/
}

void MpcGui::updateWidgets() {
	BaseGuiPlus::updateWidgets();

	// Frame counter
	/* frame_display->setVisible( pref->show_frame_counter ); */
}

#include "moc_mpcgui.cpp"

