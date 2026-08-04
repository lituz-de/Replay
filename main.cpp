#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QFileDialog>
#include <QThread>
#include <QMetaObject>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDialog>
#include <QKeyEvent>
#include <QIcon>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <atomic>

// Global Application Info
#ifndef APP_VERSION
#define APP_VERSION "v0.1"
#endif

#define APP_NAME "Replay"

enum class EventType { MouseMove, MouseDown, MouseUp, KeyDown, KeyUp };

struct ActionEvent {
    EventType type;
    int x;
    int y;
    DWORD vkCode;
    uint64_t timeMs;
};

// Global state
static HHOOK g_keyboardHook = NULL;
static HHOOK g_mouseHook = NULL;
static std::vector<ActionEvent> g_events;
static std::chrono::steady_clock::time_point g_startTime;
static std::atomic<bool> g_isRecording{false};
static std::atomic<bool> g_isPlaying{false};

// Configurable Hotkeys (Defaults: F8 and ESC)
static std::atomic<DWORD> g_recordHotkey{VK_F8};
static std::atomic<DWORD> g_abortHotkey{VK_ESCAPE};

class MainWindow;
static MainWindow* g_mainWindow = nullptr;

// Forward declarations for Win32 Hook Callbacks
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

// Helper to convert Win32 Virtual Key code to string name
static QString vkToString(DWORD vk) {
    switch (vk) {
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        case VK_ESCAPE: return "ESC";
        case VK_SPACE: return "SPACE";
        case VK_RETURN: return "ENTER";
        case VK_TAB: return "TAB";
        case VK_PAUSE: return "PAUSE";
        default: break;
    }
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    char name[128] = {0};
    if (GetKeyNameTextA(scanCode << 16, name, sizeof(name))) {
        return QString::fromLocal8Bit(name).toUpper();
    }
    return QString("0x%1").arg(vk, 0, 16).toUpper();
}

// -----------------------------------------------------------------------------
// Dynamic Theme Manager
// -----------------------------------------------------------------------------
static void applyTheme(bool isDark) {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;

    if (isDark) {
        palette.setColor(QPalette::Window, QColor(45, 45, 45));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(100, 181, 246));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        // Disabled states (Dark)
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(110, 110, 110));
    } else {
        // Explicit Light Mode Palette (Bypasses Windows OS Dark Mode)
        palette.setColor(QPalette::Window, QColor(240, 240, 240));
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(230, 230, 230));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(0, 122, 255));
        palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        palette.setColor(QPalette::HighlightedText, Qt::white);

        // Disabled states (Light)
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(210, 210, 210));
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(160, 160, 160));
    }

    qApp->setPalette(palette);
}

// -----------------------------------------------------------------------------
// Hotkey Settings Dialog
// -----------------------------------------------------------------------------
class HotkeyDialog : public QDialog {
    Q_OBJECT
public:
    HotkeyDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Hotkey Settings");
        resize(320, 160);
        setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(this);

        m_tempRecordVk = g_recordHotkey.load();
        m_tempAbortVk  = g_abortHotkey.load();

        QHBoxLayout *recLayout = new QHBoxLayout();
        recLayout->addWidget(new QLabel("Stop Record Key:", this));
        m_btnRecordKey = new QPushButton(vkToString(m_tempRecordVk), this);
        recLayout->addWidget(m_btnRecordKey);
        layout->addLayout(recLayout);

        QHBoxLayout *abortLayout = new QHBoxLayout();
        abortLayout->addWidget(new QLabel("Abort Playback Key:", this));
        m_btnAbortKey = new QPushButton(vkToString(m_tempAbortVk), this);
        abortLayout->addWidget(m_btnAbortKey);
        layout->addLayout(abortLayout);

        m_infoLabel = new QLabel("Click a button then press a key to rebind.", this);
        m_infoLabel->setStyleSheet("font-size: 11px;");
        layout->addWidget(m_infoLabel);

        QHBoxLayout *btnBox = new QHBoxLayout();
        QPushButton *btnSave = new QPushButton("Save", this);
        QPushButton *btnCancel = new QPushButton("Cancel", this);
        btnBox->addWidget(btnSave);
        btnBox->addWidget(btnCancel);
        layout->addLayout(btnBox);

        connect(m_btnRecordKey, &QPushButton::clicked, [this]() { listenForKey(true); });
        connect(m_btnAbortKey,  &QPushButton::clicked, [this]() { listenForKey(false); });
        connect(btnSave,   &QPushButton::clicked, this, &HotkeyDialog::onSave);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        DWORD vk = event->nativeVirtualKey();
        if (m_listeningRecord && vk != 0) {
            m_tempRecordVk = vk;
            m_btnRecordKey->setText(vkToString(vk));
            m_listeningRecord = false;
            m_infoLabel->setText("Record hotkey set.");
            return;
        }
        if (m_listeningAbort && vk != 0) {
            m_tempAbortVk = vk;
            m_btnAbortKey->setText(vkToString(vk));
            m_listeningAbort = false;
            m_infoLabel->setText("Abort hotkey set.");
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    void listenForKey(bool isRecord) {
        if (isRecord) {
            m_listeningRecord = true;
            m_listeningAbort = false;
            m_btnRecordKey->setText("[ Press Key... ]");
        } else {
            m_listeningAbort = true;
            m_listeningRecord = false;
            m_btnAbortKey->setText("[ Press Key... ]");
        }
        m_infoLabel->setText("Listening... Press any key on your keyboard.");
        setFocus();
    }

    void onSave() {
        g_recordHotkey.store(m_tempRecordVk);
        g_abortHotkey.store(m_tempAbortVk);
        accept();
    }

    QPushButton *m_btnRecordKey;
    QPushButton *m_btnAbortKey;
    QLabel *m_infoLabel;
    DWORD m_tempRecordVk;
    DWORD m_tempAbortVk;
    bool m_listeningRecord = false;
    bool m_listeningAbort = false;
};

// -----------------------------------------------------------------------------
// Playback Worker
// -----------------------------------------------------------------------------
class PlaybackWorker : public QObject {
    Q_OBJECT
public:
    PlaybackWorker(std::vector<ActionEvent> events) : m_events(std::move(events)) {}

public slots:
    void process() {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        uint64_t lastTime = 0;
        DWORD abortKey = g_abortHotkey.load();

        for (const auto& ev : m_events) {
            if (!g_isPlaying) break;

            if (ev.timeMs > lastTime) {
                std::this_thread::sleep_for(std::chrono::milliseconds(ev.timeMs - lastTime));
                lastTime = ev.timeMs;
            }

            if (GetAsyncKeyState(abortKey) & 0x8000) {
                break;
            }

            INPUT input = {0};
            switch (ev.type) {
                case EventType::MouseMove:
                    SetCursorPos(ev.x, ev.y);
                    break;
                case EventType::MouseDown:
                    SetCursorPos(ev.x, ev.y);
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = (ev.vkCode == VK_LBUTTON) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
                    SendInput(1, &input, sizeof(INPUT));
                    break;
                case EventType::MouseUp:
                    SetCursorPos(ev.x, ev.y);
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = (ev.vkCode == VK_LBUTTON) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
                    SendInput(1, &input, sizeof(INPUT));
                    break;
                case EventType::KeyDown:
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = static_cast<WORD>(ev.vkCode);
                    SendInput(1, &input, sizeof(INPUT));
                    break;
                case EventType::KeyUp:
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = static_cast<WORD>(ev.vkCode);
                    input.ki.dwFlags = KEYEVENTF_KEYUP;
                    SendInput(1, &input, sizeof(INPUT));
                    break;
            }
        }
        emit finished();
    }

signals:
    void finished();

private:
    std::vector<ActionEvent> m_events;
};

// -----------------------------------------------------------------------------
// Qt Main Window
// -----------------------------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        g_mainWindow = this;
        setWindowTitle(QString("%1 %2").arg(APP_NAME, APP_VERSION));
        resize(560, 380);

        setupMenuBar();

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        m_statusLabel = new QLabel("Status: Ready", this);
        mainLayout->addWidget(m_statusLabel);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        m_btnRecord = new QPushButton(this);
        m_btnStop   = new QPushButton("Stop", this);
        m_btnPlay   = new QPushButton(this);
        m_btnSave   = new QPushButton("Save...", this);
        m_btnLoad   = new QPushButton("Load...", this);
        m_btnTheme  = new QPushButton(this);

        updateButtonLabels();
        updateThemeButtonLabel();

        m_btnStop->setEnabled(false);
        m_btnPlay->setEnabled(false);
        m_btnSave->setEnabled(false);

        btnLayout->addWidget(m_btnRecord);
        btnLayout->addWidget(m_btnStop);
        btnLayout->addWidget(m_btnPlay);
        btnLayout->addWidget(m_btnSave);
        btnLayout->addWidget(m_btnLoad);
        btnLayout->addWidget(m_btnTheme);
        mainLayout->addLayout(btnLayout);

        m_logWidget = new QListWidget(this);
        mainLayout->addWidget(m_logWidget);

        connect(m_btnRecord, &QPushButton::clicked, this, &MainWindow::startRecording);
        connect(m_btnStop,   &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
        connect(m_btnPlay,   &QPushButton::clicked, this, &MainWindow::startPlayback);
        connect(m_btnSave,   &QPushButton::clicked, this, &MainWindow::saveMacro);
        connect(m_btnLoad,   &QPushButton::clicked, this, &MainWindow::loadMacro);
        connect(m_btnTheme,  &QPushButton::clicked, this, &MainWindow::toggleTheme);

        logMessage("[System] Qt Replay Engine Initialized.");
    }

    ~MainWindow() {
        if (g_isRecording) stopRecording();
    }

    void logMessage(const QString &msg) {
        m_logWidget->addItem(msg);
        m_logWidget->scrollToBottom();
    }

    void updateButtonLabels() {
        m_btnRecord->setText(QString("Record (%1)").arg(vkToString(g_recordHotkey.load())));
        m_btnPlay->setText(QString("Play (%1)").arg(vkToString(g_abortHotkey.load())));
    }

    void updateThemeButtonLabel() {
        m_btnTheme->setText(m_isDarkMode ? "🌙 Dark" : "☀️ Light");
    }

private:
    void setupMenuBar() {
        QMenuBar *mb = menuBar();

        // Settings Menu
        QMenu *settingsMenu = mb->addMenu("&Settings");
        QAction *hotkeyAction = settingsMenu->addAction("Hotkey Settings...");
        connect(hotkeyAction, &QAction::triggered, this, &MainWindow::openHotkeySettings);

        QAction *themeAction = settingsMenu->addAction("Toggle Theme");
        connect(themeAction, &QAction::triggered, this, &MainWindow::toggleTheme);

        // Help Menu
        QMenu *helpMenu = mb->addMenu("&Help");
        QAction *creditsAction = helpMenu->addAction("Credits");
        connect(creditsAction, &QAction::triggered, this, &MainWindow::showCredits);
    }

public slots:
    void toggleTheme() {
        m_isDarkMode = !m_isDarkMode;
        applyTheme(m_isDarkMode);
        updateThemeButtonLabel();
        logMessage(QString("[UI] Switched to %1 theme.").arg(m_isDarkMode ? "Dark" : "Light"));
    }

    void openHotkeySettings() {
        HotkeyDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            updateButtonLabels();
            logMessage(QString("[Settings] Hotkeys updated: Record/Stop = %1 | Abort = %2")
            .arg(vkToString(g_recordHotkey.load()))
            .arg(vkToString(g_abortHotkey.load())));
        }
    }

    void showCredits() {
        QString text = QString(
            "<h2 style='margin-bottom: 2px;'>%1 <span style='font-size: 13px; font-weight: normal;'>%2</span></h2>"
            "<p style='margin-top: 4px; margin-bottom: 8px;'>A lightweight low-level mouse and keyboard input recording and playback engine for Windows.</p>"
            "<hr>"
            "<p style='margin-top: 6px; margin-bottom: 6px;'>Qt 6 - (GNU LGPL v3 License)</p>"
            "<hr>"
            "<p style='margin-top: 6px; margin-bottom: 2px;'><a href='https://www.github.com/lituz-de/Replay'>www.github.com/lituz-de/Replay</a></p>"
            "<p style='margin-top: 0px;'>MIT License</p>"
        ).arg(APP_NAME, APP_VERSION);

        QMessageBox::about(this, "Credits", text);
    }

    void startRecording() {
        g_events.clear();
        g_startTime = std::chrono::steady_clock::now();
        g_isRecording = true;

        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);

        m_btnRecord->setEnabled(false);
        m_btnStop->setEnabled(true);
        m_btnPlay->setEnabled(false);
        m_btnSave->setEnabled(false);
        m_btnLoad->setEnabled(false);

        m_statusLabel->setText(QString("Status: RECORDING... (Press %1 to Stop)").arg(vkToString(g_recordHotkey.load())));
        logMessage("[Record] Global hooks active. Recording started.");
    }

    void stopRecording() {
        if (!g_isRecording) return;
        g_isRecording = false;

        if (g_keyboardHook) { UnhookWindowsHookEx(g_keyboardHook); g_keyboardHook = NULL; }
        if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL; }

        m_btnRecord->setEnabled(true);
        m_btnStop->setEnabled(false);
        m_btnPlay->setEnabled(!g_events.empty());
        m_btnSave->setEnabled(!g_events.empty());
        m_btnLoad->setEnabled(true);

        m_statusLabel->setText("Status: Ready");
        logMessage(QString("[Record] Stopped. Captured %1 events.").arg(g_events.size()));
    }

    void onStopButtonClicked() {
        if (g_isRecording) stopRecording();
        if (g_isPlaying) g_isPlaying = false;
    }

    void startPlayback() {
        if (g_events.empty()) return;

        g_isPlaying = true;

        m_btnRecord->setEnabled(false);
        m_btnStop->setEnabled(true);
        m_btnPlay->setEnabled(false);
        m_btnSave->setEnabled(false);
        m_btnLoad->setEnabled(false);

        m_statusLabel->setText(QString("Status: PLAYBACK... (Hold %1 to Abort)").arg(vkToString(g_abortHotkey.load())));
        logMessage("[Playback] Starting in 2 seconds...");

        QThread *thread = new QThread();
        PlaybackWorker *worker = new PlaybackWorker(g_events);
        worker->moveToThread(thread);
        connect(thread, &QThread::started, worker, &PlaybackWorker::process);
        connect(worker, &PlaybackWorker::finished, thread, &QThread::quit);
        connect(worker, &PlaybackWorker::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        connect(thread, &QThread::finished, this, &MainWindow::onPlaybackFinished);

        thread->start();
    }

    void onPlaybackFinished() {
        g_isPlaying = false;

        m_btnRecord->setEnabled(true);
        m_btnStop->setEnabled(false);
        m_btnPlay->setEnabled(true);
        m_btnSave->setEnabled(true);
        m_btnLoad->setEnabled(true);

        m_statusLabel->setText("Status: Ready");
        logMessage("[Playback] Finished.");
    }

    void saveMacro() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Macro", "", "Binary Files (*.bin)");
        if (fileName.isEmpty()) return;

        std::ofstream out(fileName.toStdString(), std::ios::binary);
        if (!out) {
            logMessage("[Error] Failed to open file for writing.");
            return;
        }

        size_t count = g_events.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        out.write(reinterpret_cast<const char*>(g_events.data()), count * sizeof(ActionEvent));
        logMessage(QString("[Save] Saved %1 events to file.").arg(count));
    }

    void loadMacro() {
        QString fileName = QFileDialog::getOpenFileName(this, "Load Macro", "", "Binary Files (*.bin)");
        if (fileName.isEmpty()) return;

        std::ifstream in(fileName.toStdString(), std::ios::binary);
        if (!in) {
            logMessage("[Error] Could not load macro file.");
            return;
        }

        size_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        g_events.resize(count);
        in.read(reinterpret_cast<char*>(g_events.data()), count * sizeof(ActionEvent));

        m_btnPlay->setEnabled(!g_events.empty());
        m_btnSave->setEnabled(!g_events.empty());
        logMessage(QString("[Load] Loaded %1 events from file.").arg(count));
    }

private:
    QLabel *m_statusLabel;
    QPushButton *m_btnRecord;
    QPushButton *m_btnStop;
    QPushButton *m_btnPlay;
    QPushButton *m_btnSave;
    QPushButton *m_btnLoad;
    QPushButton *m_btnTheme;
    QListWidget *m_logWidget;
    bool m_isDarkMode = true;
};

// -----------------------------------------------------------------------------
// Low-Level Win32 Hook Callbacks
// -----------------------------------------------------------------------------
static uint64_t GetElapsedMs() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - g_startTime).count();
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_isRecording) {
        KBDLLHOOKSTRUCT* pKbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        // Check configurable Record Stop Hotkey
        if (pKbd->vkCode == g_recordHotkey.load() && wParam == WM_KEYDOWN) {
            if (g_mainWindow) {
                QMetaObject::invokeMethod(g_mainWindow, &MainWindow::stopRecording, Qt::QueuedConnection);
            }
            return 1;
        }

        ActionEvent ev{};
        ev.vkCode = pKbd->vkCode;
        ev.timeMs = GetElapsedMs();

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            ev.type = EventType::KeyDown;
            g_events.push_back(ev);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            ev.type = EventType::KeyUp;
            g_events.push_back(ev);
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_isRecording) {
        MSLLHOOKSTRUCT* pMouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        ActionEvent ev{};
        ev.x = pMouse->pt.x;
        ev.y = pMouse->pt.y;
        ev.timeMs = GetElapsedMs();

        switch (wParam) {
            case WM_MOUSEMOVE:
                if (g_events.empty() || g_events.back().x != ev.x || g_events.back().y != ev.y) {
                    ev.type = EventType::MouseMove;
                    g_events.push_back(ev);
                }
                break;
            case WM_LBUTTONDOWN:
                ev.type = EventType::MouseDown;
                ev.vkCode = VK_LBUTTON;
                g_events.push_back(ev);
                break;
            case WM_LBUTTONUP:
                ev.type = EventType::MouseUp;
                ev.vkCode = VK_LBUTTON;
                g_events.push_back(ev);
                break;
            case WM_RBUTTONDOWN:
                ev.type = EventType::MouseDown;
                ev.vkCode = VK_RBUTTON;
                g_events.push_back(ev);
                break;
            case WM_RBUTTONUP:
                ev.type = EventType::MouseUp;
                ev.vkCode = VK_RBUTTON;
                g_events.push_back(ev);
                break;
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Default to dark theme on startup
    applyTheme(true);

    // Set window & taskbar icon
    app.setWindowIcon(QIcon("app.ico"));

    MainWindow window;
    window.show();
    return app.exec();
}
