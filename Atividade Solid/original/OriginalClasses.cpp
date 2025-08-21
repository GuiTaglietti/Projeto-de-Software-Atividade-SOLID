/**
 * @author Guilherme M. Taglietti
 * @brief  Banco simples com operações de depósito, saque, transferência e extrato.
 * @date   2023-10-01
 * @note   Atividade - Projeto de Software (VERSÃO ORIGINAL).
 */
#include <bits/stdc++.h>
using namespace std;

struct ValorInvalido : runtime_error { using runtime_error::runtime_error; };
struct SaldoInsuficiente : runtime_error { using runtime_error::runtime_error; };
struct EntidadeNaoEncontrada : runtime_error { using runtime_error::runtime_error; };

struct Cliente {
    int id;
    string nome;
    string cpf;
};

struct Transacao {
    chrono::system_clock::time_point momento;
    string tipo;
    long long centavos;
    string descricao;
    string origem;
    string destino;
};

static string format_money(long long centavos) {
    bool neg = centavos < 0;
    long long v = llabs(centavos);
    long long reais = v / 100;
    int c = v % 100;
    stringstream ss;
    if (neg) ss << "-";
    ss << reais << "." << setw(2) << setfill('0') << c;
    return ss.str();
}

class Conta {
    string numero_;
    shared_ptr<Cliente> cliente_;
    long long centavos_ = 0;
    vector<Transacao> historico_;
public:
    Conta(string numero, shared_ptr<Cliente> cliente) : numero_(move(numero)), cliente_(move(cliente)) {}
    const string& numero() const { return numero_; }
    shared_ptr<Cliente> cliente() const { return cliente_; }
    long long saldo_centavos() const { return centavos_; }
    const vector<Transacao>& historico() const { return historico_; }
    void depositar(long long centavos, const string& descricao = "") {
        if (centavos <= 0) throw ValorInvalido("valor deve ser positivo");
        centavos_ += centavos;
        historico_.push_back({chrono::system_clock::now(),"DEPOSITO",centavos,descricao,"",numero_});
    }
    void sacar(long long centavos, const string& descricao = "") {
        if (centavos <= 0) throw ValorInvalido("valor deve ser positivo");
        if (centavos_ < centavos) throw SaldoInsuficiente("saldo insuficiente");
        centavos_ -= centavos;
        historico_.push_back({chrono::system_clock::now(),"SAQUE",centavos,descricao,numero_,""});
    }
    void registrar(const Transacao& t) { historico_.push_back(t); }
};

class Banco {
    unordered_map<string, shared_ptr<Cliente>> clientes_;
    unordered_map<string, unique_ptr<Conta>> contas_;
    int seq_cliente_ = 1;
    int seq_conta_ = 1001;
    static long long to_centavos(double valor) {
        long long c = llround(valor * 100.0);
        if (c == 0) throw ValorInvalido("valor inválido");
        return c;
    }
public:
    shared_ptr<Cliente> criar_cliente(const string& nome, const string& cpf) {
        auto it = clientes_.find(cpf);
        if (it != clientes_.end()) return it->second;
        auto c = make_shared<Cliente>(Cliente{seq_cliente_++, nome, cpf});
        clientes_[cpf] = c;
        return c;
    }
    Conta* abrir_conta(const string& cpf) {
        auto it = clientes_.find(cpf);
        if (it == clientes_.end()) throw EntidadeNaoEncontrada("cliente não encontrado");
        string numero = to_string(seq_conta_++);
        contas_[numero] = make_unique<Conta>(numero, it->second);
        return contas_[numero].get();
    }
    Conta* buscar_conta(const string& numero) {
        auto it = contas_.find(numero);
        if (it == contas_.end()) throw EntidadeNaoEncontrada("conta não encontrada");
        return it->second.get();
    }
    void depositar(const string& numero, double valor) {
        buscar_conta(numero)->depositar(to_centavos(valor), "depósito");
    }
    void sacar(const string& numero, double valor) {
        buscar_conta(numero)->sacar(to_centavos(valor), "saque");
    }
    void transferir(const string& origem, const string& destino, double valor) {
        if (origem == destino) throw ValorInvalido("contas iguais");
        long long c = to_centavos(valor);
        auto* co = buscar_conta(origem);
        auto* cd = buscar_conta(destino);
        co->sacar(c, string("transferência para ") + destino);
        cd->depositar(c, string("transferência de ") + origem);
        Transacao t{chrono::system_clock::now(),"TRANSFERENCIA",c,"transferência",origem,destino};
        co->registrar(t);
        cd->registrar(t);
    }
    vector<pair<shared_ptr<Cliente>, vector<Conta*>>> listar_clientes_e_contas() {
        unordered_map<string, vector<Conta*>> mapc;
        for (auto& kv : contas_) mapc[kv.second->cliente()->cpf].push_back(kv.second.get());
        vector<pair<shared_ptr<Cliente>, vector<Conta*>>> out;
        for (auto& kv : clientes_) out.push_back({kv.second, mapc[kv.first]});
        return out;
    }
    vector<Transacao> extrato(const string& numero) {
        return buscar_conta(numero)->historico();
    }
    static Banco criar_dados_mock() {
        Banco b;
        auto a = b.criar_cliente("Ana","11111111111");
        auto c1 = b.abrir_conta(a->cpf)->numero();
        auto br = b.criar_cliente("Bruno","22222222222");
        auto c2 = b.abrir_conta(br->cpf)->numero();
        auto ca = b.criar_cliente("Carla","33333333333");
        auto c3 = b.abrir_conta(ca->cpf)->numero();
        b.depositar(c1, 1500.00);
        b.depositar(c2, 800.00);
        b.depositar(c3, 2500.00);
        b.transferir(c3, c1, 300.00);
        b.sacar(c2, 100.00);
        return b;
    }
};

Banco criar_dados_mock() {
    return Banco::criar_dados_mock();
}

int main() {
    auto banco = criar_dados_mock();
    auto lista = banco.listar_clientes_e_contas();
    for (auto& par : lista) {
        auto cli = par.first;
        cout << cli->id << " " << cli->nome << " " << cli->cpf << " -> ";
        vector<string> contas;
        for (auto* c : par.second) contas.push_back(c->numero() + ":" + format_money(c->saldo_centavos()));
        cout << "[" << (contas.empty() ? "" : contas[0]);
        for (size_t i=1;i<contas.size();++i) cout << ", " << contas[i];
        cout << "]\n";
    }
    if (!lista.empty() && !lista.front().second.empty()) {
        auto num = lista.front().second.front()->numero();
        auto ext = banco.extrato(num);
        for (auto& t : ext) {
            cout << num << " " << t.tipo << " " << format_money(t.centavos) << " " << t.descricao << "\n";
        }
    }
    return 0;
}
