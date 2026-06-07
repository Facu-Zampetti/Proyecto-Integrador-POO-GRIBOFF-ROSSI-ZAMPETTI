#include "utils/StyleManager.h"
#include <QFile>
#include <QTextStream>

StyleManager* StyleManager::s_instance = nullptr;

StyleManager* StyleManager::instance()
{
    if (!s_instance)
        s_instance = new StyleManager();
    return s_instance;
}

StyleManager::StyleManager(QObject* parent)
    : QObject(parent)
{
}

QString StyleManager::loadStyleSheet(const QString& resourcePath) const
{
    QFile file(resourcePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }
    // Fallback al tema embebido si no se puede abrir el archivo
    return darkTheme();
}

QString StyleManager::darkTheme()
{
    return QString(R"(

/* ══════════════════════════════════════════════════════
   CARLENS — Dark Theme  (GitHub Dark style)
   ══════════════════════════════════════════════════════ */

/* ── Global ─────────────────────────────────────────── */
QWidget {
    background-color: #0d1117;
    color:            #e6edf3;
    font-family:      "Segoe UI", "Inter", sans-serif;
    font-size:        13px;
}

QWidget[class="card"] {
    background-color: #161b22;
    border:           1px solid #30363d;
    border-radius:    8px;
    padding:          16px;
}

/* ── QLabel ──────────────────────────────────────────── */
QLabel {
    color:      #e6edf3;
    background: transparent;
}

QLabel[class="title"] {
    font-size:   22px;
    font-weight: 700;
    color:       #58a6ff;
}

QLabel[class="subtitle"] {
    font-size: 13px;
    color:     #8b949e;
}

QLabel[class="error"] {
    color:     #f85149;
    font-size: 12px;
}

QLabel[class="success"] {
    color:     #3fb950;
    font-size: 12px;
}

QLabel[class="badge"] {
    background-color: #21262d;
    border:           1px solid #30363d;
    border-radius:    12px;
    padding:          2px 10px;
    font-size:        12px;
    color:            #8b949e;
}

/* ── QLineEdit ───────────────────────────────────────── */
QLineEdit {
    background-color: #0d1117;
    color:            #e6edf3;
    border:           1px solid #30363d;
    border-radius:    6px;
    padding:          8px 12px;
    font-size:        13px;
    selection-background-color: #264f78;
}

QLineEdit:focus {
    border: 1px solid #58a6ff;
    outline: none;
}

QLineEdit:disabled {
    background-color: #161b22;
    color:            #484f58;
}

QLineEdit[class="search"] {
    padding-left: 32px;
}

/* ── QPushButton ─────────────────────────────────────── */
QPushButton {
    background-color: #21262d;
    color:            #e6edf3;
    border:           1px solid #30363d;
    border-radius:    6px;
    padding:          8px 16px;
    font-size:        13px;
    font-weight:      500;
}

QPushButton:hover {
    background-color: #30363d;
    border-color:     #8b949e;
}

QPushButton:pressed {
    background-color: #161b22;
}

QPushButton:disabled {
    background-color: #161b22;
    color:            #484f58;
    border-color:     #21262d;
}

QPushButton[class="primary"] {
    background-color: #238636;
    color:            #ffffff;
    border-color:     #2ea043;
    font-weight:      600;
}

QPushButton[class="primary"]:hover {
    background-color: #2ea043;
}

QPushButton[class="primary"]:pressed {
    background-color: #238636;
}

QPushButton[class="accent"] {
    background-color: #1f6feb;
    color:            #ffffff;
    border-color:     #388bfd;
    font-weight:      600;
}

QPushButton[class="accent"]:hover {
    background-color: #388bfd;
}

QPushButton[class="danger"] {
    background-color: #da3633;
    color:            #ffffff;
    border-color:     #f85149;
}

QPushButton[class="danger"]:hover {
    background-color: #b91c1c;
}

QPushButton[class="sidebar-btn"] {
    background-color: transparent;
    border:           none;
    border-radius:    6px;
    text-align:       left;
    padding:          10px 16px;
    color:            #8b949e;
    font-size:        13px;
}

QPushButton[class="sidebar-btn"]:hover {
    background-color: #161b22;
    color:            #e6edf3;
}

QPushButton[class="sidebar-btn"][active="true"] {
    background-color: #1f2d40;
    color:            #58a6ff;
    border-left:      3px solid #58a6ff;
}

/* ── QTextEdit ───────────────────────────────────────── */
QTextEdit {
    background-color: #0d1117;
    color:            #e6edf3;
    border:           1px solid #30363d;
    border-radius:    6px;
    padding:          8px;
    font-size:        13px;
    selection-background-color: #264f78;
}

QTextEdit:focus {
    border: 1px solid #58a6ff;
}

/* ── QScrollArea ─────────────────────────────────────── */
QScrollArea {
    border:           none;
    background-color: transparent;
}

QScrollArea > QWidget > QWidget {
    background-color: transparent;
}

/* ── QScrollBar ──────────────────────────────────────── */
QScrollBar:vertical {
    background-color: #161b22;
    width:            8px;
    margin:           0;
    border-radius:    4px;
}

QScrollBar::handle:vertical {
    background-color: #30363d;
    border-radius:    4px;
    min-height:       20px;
}

QScrollBar::handle:vertical:hover {
    background-color: #8b949e;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background-color: #161b22;
    height:           8px;
    border-radius:    4px;
}

QScrollBar::handle:horizontal {
    background-color: #30363d;
    border-radius:    4px;
    min-width:        20px;
}

/* ── QProgressBar ────────────────────────────────────── */
QProgressBar {
    background-color: #21262d;
    border:           1px solid #30363d;
    border-radius:    6px;
    height:           8px;
    text-align:       center;
    color:            transparent;
}

QProgressBar::chunk {
    background-color: #1f6feb;
    border-radius:    5px;
}

/* ── QCheckBox ───────────────────────────────────────── */
QCheckBox {
    color:   #e6edf3;
    spacing: 8px;
}

QCheckBox::indicator {
    width:         16px;
    height:        16px;
    border:        1px solid #30363d;
    border-radius: 4px;
    background:    #0d1117;
}

QCheckBox::indicator:checked {
    background:   #1f6feb;
    border-color: #388bfd;
}

/* ── QFrame separador ────────────────────────────────── */
QFrame[frameShape="4"],
QFrame[frameShape="5"] {
    color: #30363d;
}

/* ── QSplitter ───────────────────────────────────────── */
QSplitter::handle {
    background-color: #30363d;
}

/* ── QStackedWidget ──────────────────────────────────── */
QStackedWidget {
    background-color: #0d1117;
    border: none;
}

/* ── Sidebar ─────────────────────────────────────────── */
#sidebar {
    background-color: #010409;
    border-right:     1px solid #21262d;
    min-width:        200px;
    max-width:        240px;
}

/* ── TopBar ──────────────────────────────────────────── */
#topbar {
    background-color: #161b22;
    border-bottom:    1px solid #21262d;
    min-height:       48px;
    max-height:       48px;
}

/* ── StatusBar ───────────────────────────────────────── */
#statusBar {
    background-color: #161b22;
    border-top:       1px solid #21262d;
    min-height:       34px;
    max-height:       34px;
}

#statusBar QLabel {
    font-size: 13px;
    color:     #8b949e;
}

/* ── VehicleCard ─────────────────────────────────────── */
#vehicleCard {
    background-color: #161b22;
    border:           1px solid #30363d;
    border-radius:    8px;
}

#vehicleCard:hover {
    border-color:     #58a6ff;
}

/* ── LoginWidget ─────────────────────────────────────── */
#loginCard {
    background-color: #161b22;
    border:           1px solid #30363d;
    border-radius:    12px;
    padding:          40px;
    min-width:        380px;
    max-width:        420px;
}

/* ── Tooltip ─────────────────────────────────────────── */
QToolTip {
    background-color: #161b22;
    color:            #e6edf3;
    border:           1px solid #30363d;
    border-radius:    4px;
    padding:          4px 8px;
    font-size:        12px;
}

)");
}
