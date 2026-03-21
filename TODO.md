# TODO

## Funcionalidades
- [x] Verificação de CRC32 no servidor
- [x] Retransmissão de pacote se CRC32 não bater
- [ ] Argumentos na execução
  - Servidor: ip, porta, caminho de destino
  - Cliente: ip, porta (opcional) caminho pro arquivo
- [ ] DEBUG MODE server-side principalmente pra printar CRC calculado / recebido, pacotes e etc

## Qualidade de código
- [x] Refatorar write_new_file (recursão → loop)
- [x] Padronização de nomenclatura
- [x] Código Python em POO

## Documentação
- [ ] Documentar o protocolo
- [ ] README com instruções de uso
- [ ] LOG file com informações uteis
