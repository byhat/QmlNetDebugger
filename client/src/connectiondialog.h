#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QUrl>

#include "settings.h"

/**
 * @brief Inline connection dialog overlay for configuring server connection
 * 
 * Provides UI for entering server IP, port, and QML filename.
 * Displayed as an overlay within the main window instead of a separate window,
 * which is required for proper behavior on Android.
 * Also manages connection history and saves settings.
 */
class ConnectionDialog : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param settings Application settings
     * @param parent Parent widget (should be the main window's central widget)
     */
    explicit ConnectionDialog(Settings *settings, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ConnectionDialog() override;

    /**
     * @brief Get the configured server URL
     * @return Server URL
     */
    QUrl serverUrl() const;

    /**
     * @brief Get the configured QML filename
     * @return QML filename
     */
    QString qmlFilename() const;

    /**
     * @brief Get the configured update interval
     * @return Update interval in milliseconds
     */
    int updateInterval() const;

    /**
     * @brief Check if SSE should be used
     * @return True if SSE is enabled
     */
    bool useSSE() const;

    /**
     * @brief Show the dialog overlay
     * 
     * Resizes to fill the parent, reloads settings, and makes visible.
     */
    void showDialog();

    /**
     * @brief Hide the dialog overlay
     */
    void hideDialog();

signals:
    /**
     * @brief Emitted when the user clicks Connect and input is valid
     */
    void accepted();

    /**
     * @brief Emitted when the user clicks Cancel or presses Escape
     */
    void rejected();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onConnectClicked();
    void onCancelClicked();
    void onRecentConnectionSelected(int index);
    bool validateInput();

private:
    void setupUi();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void loadRecentConnections();

    // UI Components
    QFrame *m_panel;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpinBox;
    QLineEdit *m_filenameEdit;
    QComboBox *m_recentCombo;
    QCheckBox *m_useSSECheckBox;
    QSpinBox *m_updateIntervalSpinBox;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;

    // Settings reference
    Settings *m_settings;
};

#endif // CONNECTIONDIALOG_H
