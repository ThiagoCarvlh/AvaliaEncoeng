#ifndef PAGINAAVALIADORES_H
#define PAGINAAVALIADORES_H

#include <QWidget>
#include <QString>

class QTableView;
class QStandardItemModel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QLabel;
class AvaliadorFilterModel;

namespace Ui { class PaginaAvaliadores; }

class PaginaAvaliadores : public QWidget {
    Q_OBJECT
public:
    explicit PaginaAvaliadores(QWidget* parent = nullptr);
    ~PaginaAvaliadores();

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onRecarregar();


    void onBuscaChanged(const QString& texto);
    void onCategoriaChanged(int index);
    void onExportCsv();

private:
    Ui::PaginaAvaliadores* ui;

    QTableView*            m_table{};
    QStandardItemModel*    m_model{};
    AvaliadorFilterModel*  m_filter{};
    QPushButton *m_btnNovo{}, *m_btnEditar{}, *m_btnRemover{}, *m_btnRecarregar{}, *m_btnExportCsv{};
    QLineEdit*  m_editBusca{};
    QComboBox*  m_comboCategoria{};
    QLabel*     m_labelTotal{};
    int         m_nextId{1};

    const QString m_arquivo = "avaliadores.csv";


    bool salvarNoArquivo() const;
    bool carregarDoArquivo();
    void recomputarNextId();

    int  selectedRow() const;
    void addAvaliador(const QString& nome,
                      const QString& email,
                      const QString& cpf,
                      const QString& categoria,
                      const QString& senha);

    void atualizarTotal();
};

#endif // PAGINAAVALIADORES_H
