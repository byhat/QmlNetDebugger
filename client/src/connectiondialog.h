#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QUrl>

#include "settings.h"

/**
 * @brief Startup dialog for configuring server connection
 * 
 * Provides UI for entering server IP, port, and QML filename.
 * Also manages connection history and saves settings.
 */
class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     * @param settings Application settings
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

private slots:
    /**
     * @brief Handle connect button click
     */
    void onConnectClicked();

    /**
     * @brief Handle cancel button click
     */
    void onCancelClicked();

    /**
     * @brief Handle recent connection selection
     */
    void onRecentConnectionSelected(int index);

    /**
     * @brief Validate input fields
     * @return True if input is valid
     */
    bool validateInput();

private:
    /**
     * @brief Setup UI components
     */
    void setupUi();

    /**
     * @brief Setup connections
     */
    void setupConnections();

    /**
     * @brief Load settings into UI
     */
    void loadSettings();

    /**
     * @brief Save settings from UI
     */
    void saveSettings();

    /**
     * @brief Load recent connections into combo box
     */
    void loadRecentConnections();

    // UI Components
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
