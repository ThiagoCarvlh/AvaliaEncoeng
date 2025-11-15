#pragma once

#include <QWidget>
#include <QString>

// Forward declarations
class QTableView;
class QStandardItemModel;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class ProjetoFilterModel;

namespace Ui {
class PaginaProjetos;
}

class PaginaProjetos : public QWidget
{
    Q_OBJECT

public:
    explicit PaginaProjetos(QWidget* parent = nullptr);
    ~PaginaProjetos();

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onRecarregar();
    void onExportCsv();

    // filtros
    void onBuscaChanged(const QString& texto);
    void onCategoriaChanged(int index);

private:
    // Métodos privados
    bool salvarNoArquivo() const;
    bool carregarDoArquivo();
    void recomputarNextId();
    void addProjeto(const QString& nome,
                    const QString& desc,
                    const QString& resp,
                    const QString& categoria);
    int  selectedRow() const;
    void atualizarTotal();

    // Membros da UI
    Ui::PaginaProjetos* ui;

    // Modelo e Visão
    QTableView*         m_table{};
    QStandardItemModel* m_model{};
    ProjetoFilterModel* m_filter{};

    // Widgets
    QPushButton* m_btnNovo{};
    QPushButton* m_btnEditar{};
    QPushButton* m_btnRemover{};
    QPushButton* m_btnRecarregar{};
    QPushButton* m_btnExportCsv{};
    QLabel*      m_labelTotal{};
    QLineEdit*   m_editBusca{};
    QComboBox*   m_comboCategoria{};

    // Dados
    int         m_nextId{1};
    const QString m_arquivo = "projetos.txt";
};
