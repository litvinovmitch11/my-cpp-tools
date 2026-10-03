export const INITIAL_CODE = `int f(int x) {
  if (x > 2)
    return (x / 42);
  return x + 1;
}

int g() {
  int b = 0;
  for (int i = 0; i < 123; ++i) {
    b += 3 * i + 2;
  }
  return b;
}
`;

export const SAMPLE_DOT = `digraph AST {
  rankdir=TB;
  graph [fontname="DejaVu Sans Mono"];
  node [shape=box, style=filled, fontname="DejaVu Sans Mono", fontsize=10];
  subgraph cluster_0 {
    label="f";
    color="#CBD5E1";
    n0 [label="Function f", fillcolor="#DCEBFA"];
    n1 [label="ParmVar x", fillcolor="#DCEBFA"];
    n0 -> n1;
    n2 [label="CompoundStmt", fillcolor="#E2F0D9"];
    n0 -> n2;
    n3 [label="IfStmt", fillcolor="#E2F0D9"];
    n2 -> n3;
    n4 [label="BinaryOperator >", fillcolor="#FFF2CC"];
    n3 -> n4;
    n5 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n4 -> n5;
    n6 [label="DeclRefExpr x", fillcolor="#FFF2CC"];
    n5 -> n6;
    n7 [label="IntegerLiteral 2", fillcolor="#FFF2CC"];
    n4 -> n7;
    n8 [label="ReturnStmt", fillcolor="#E2F0D9"];
    n3 -> n8;
    n9 [label="ParenExpr", fillcolor="#FFF2CC"];
    n8 -> n9;
    n10 [label="BinaryOperator /", fillcolor="#FFF2CC"];
    n9 -> n10;
    n11 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n10 -> n11;
    n12 [label="DeclRefExpr x", fillcolor="#FFF2CC"];
    n11 -> n12;
    n13 [label="IntegerLiteral 42", fillcolor="#FFF2CC"];
    n10 -> n13;
    n14 [label="ReturnStmt", fillcolor="#E2F0D9"];
    n2 -> n14;
    n15 [label="BinaryOperator +", fillcolor="#FFF2CC"];
    n14 -> n15;
    n16 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n15 -> n16;
    n17 [label="DeclRefExpr x", fillcolor="#FFF2CC"];
    n16 -> n17;
    n18 [label="IntegerLiteral 1", fillcolor="#FFF2CC"];
    n15 -> n18;
  }
  subgraph cluster_1 {
    label="g";
    color="#CBD5E1";
    n19 [label="Function g", fillcolor="#DCEBFA"];
    n20 [label="CompoundStmt", fillcolor="#E2F0D9"];
    n19 -> n20;
    n21 [label="DeclStmt", fillcolor="#E2F0D9"];
    n20 -> n21;
    n22 [label="Var b", fillcolor="#DCEBFA"];
    n21 -> n22;
    n23 [label="IntegerLiteral 0", fillcolor="#FFF2CC"];
    n22 -> n23;
    n24 [label="ForStmt", fillcolor="#E2F0D9"];
    n20 -> n24;
    n25 [label="DeclStmt", fillcolor="#E2F0D9"];
    n24 -> n25;
    n26 [label="Var i", fillcolor="#DCEBFA"];
    n25 -> n26;
    n27 [label="IntegerLiteral 0", fillcolor="#FFF2CC"];
    n26 -> n27;
    n28 [label="BinaryOperator <", fillcolor="#FFF2CC"];
    n24 -> n28;
    n29 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n28 -> n29;
    n30 [label="DeclRefExpr i", fillcolor="#FFF2CC"];
    n29 -> n30;
    n31 [label="IntegerLiteral 123", fillcolor="#FFF2CC"];
    n28 -> n31;
    n32 [label="UnaryOperator ++", fillcolor="#FFF2CC"];
    n24 -> n32;
    n33 [label="DeclRefExpr i", fillcolor="#FFF2CC"];
    n32 -> n33;
    n34 [label="CompoundStmt", fillcolor="#E2F0D9"];
    n24 -> n34;
    n35 [label="CompoundAssignOperator +=", fillcolor="#FFF2CC"];
    n34 -> n35;
    n36 [label="DeclRefExpr b", fillcolor="#FFF2CC"];
    n35 -> n36;
    n37 [label="BinaryOperator +", fillcolor="#FFF2CC"];
    n35 -> n37;
    n38 [label="BinaryOperator *", fillcolor="#FFF2CC"];
    n37 -> n38;
    n39 [label="IntegerLiteral 3", fillcolor="#FFF2CC"];
    n38 -> n39;
    n40 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n38 -> n40;
    n41 [label="DeclRefExpr i", fillcolor="#FFF2CC"];
    n40 -> n41;
    n42 [label="IntegerLiteral 2", fillcolor="#FFF2CC"];
    n37 -> n42;
    n43 [label="ReturnStmt", fillcolor="#E2F0D9"];
    n20 -> n43;
    n44 [label="ImplicitCastExpr", fillcolor="#FFF2CC"];
    n43 -> n44;
    n45 [label="DeclRefExpr b", fillcolor="#FFF2CC"];
    n44 -> n45;
  }
}`;
