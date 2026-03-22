# TODO

## Funcionalidades
- [x] Verificação de CRC32 no servidor
- [x] Retransmissão de pacote se CRC32 não bater
- [x] Argumentos na execução
  - Servidor: ip, porta, caminho de destino
  - Cliente: ip, porta (opcional) caminho pro arquivo
- [x] DEBUG MODE server-side principalmente pra printar CRC calculado / recebido, pacotes e etc
- [ ] Tratar erro de conexão no meio da transferencia
- [ ] Multithread
- [ ] Tratar arquivos de mesmo nome excluem o anterior
- [ ] Checksum do arquivo inteiro no MSG_DONE

## Qualidade de código
- [x] Refatorar write_new_file (recursão → loop)
- [x] Padronização de nomenclatura
- [x] Código Python em POO

## Documentação
- [x] Documentar o protocolo
- [x] README com instruções de uso
- [ ] LOG file com informações uteis
