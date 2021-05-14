#include "extern.h"
void codeLog(char *msg)
{
#ifdef InterCodeDebug
    printf("%s\n", msg);
#endif
}

char *int2String(int num, char *str)
{
    int i = 0;
    if (num < 0)
    {
        num = -num;
        str[i++] = '-';
    }
    do
    {
        str[i++] = num % 10 + 48;
        num /= 10;
    } while (num);
    str[i] = '\0';
    int j = 0;
    if (str[0] == '-')
    {
        j = 1;
        ++i;
    }
    for (; j < i / 2; j++)
    {
        str[j] = str[j] + str[i - 1 - j];
        str[i - 1 - j] = str[j] - str[i - 1 - j];
        str[j] = str[j] - str[i - 1 - j];
    }
    return str;
}

int calculateSize(Type type)
{
    if (type->kind == BASIC)
        return 4;
    else if (type->kind == ARRAY)
        return type->u.array.size * calculateSize(type->u.array.elem);
    else if (type->kind == STRUCTVAR)
    {
        int size = 0;
        FieldList p = type->u.structure;
        while (p)
        {
            size += calculateSize(p->type);
            p = p->tail;
        }
        return size;
    }
}

void insertCode(InterCode code)
{
    if (head == NULL)
    {
        head = code;
        tail = code;
        head->next = head->prev = NULL;
    }
    else
    {
        tail->next = code;
        code->prev = tail;
        code->next = NULL;
        tail = tail->next;
    }
}


InterCode createCode()
{
    InterCode code = (InterCode)malloc(sizeof(struct InterCode_));
    code->next = code->prev = NULL;
    return code;
}

Operand createOpTmp()
{
    tNum++;
    Operand res = (Operand)malloc(sizeof(struct Operand_));
    res->kind = TEMPVAR;
    strcpy(res->u.value, "tt");
    char str[CHARMAXSIZE];
    int2String(tNum, str);
    strcat(res->u.value, str);
    return res;
}

Operand createOpLabel()
{
    labelNum++;
    Operand res = (Operand)malloc(sizeof(struct Operand_));
    res->kind = LABEL;
    strcpy(res->u.value, "label");
    char str[CHARMAXSIZE];
    int2String(labelNum, str);
    strcat(res->u.value, str);
    return res;
}
Operand copyOpLabel(int num)
{
    Operand res = (Operand)malloc(sizeof(struct Operand_));
    res->kind = LABEL;
    strcpy(res->u.value, "label");
    char str[CHARMAXSIZE];
    int2String(num, str);
    strcat(res->u.value, str);
    return res;
}
void initInterCode(node root)
{
    head = tail = NULL;
    tNum = labelNum = 0;
    translateProgram(root);
}

void translateProgram(node root)
{
    translateExtDefList(root->child);
}

void translateExtDefList(node root)
{
    if (root == NULL)
        return;
    translateExtDef(root->child);
    translateExtDefList(root->child->sibling);
}

void translateExtDef(node root)
{
    node n1 = getKChild(root, 1);
    node n2 = getKChild(root, 2);
    if (strcmp(n1->name, "FunDec") == 0)
    {
        translateFunDec(n1);
        translateCompst(n2);
    }
}

//REVISE FUNCTION -> MYFUNCTION
void translateFunDec(node root)
{
    Operand op_tmp = (Operand)malloc(sizeof(struct Operand_));
    op_tmp->kind = VARIABLE;
    strcpy(op_tmp->u.value, root->child->val);
    InterCode code = createCode();
    code->kind = MYFUNCTION;
    code->u.op_single.op = op_tmp;
    insertCode(code);
    if (getChildNum(root) == 4)
        translateVarList(getKChild(root, 2));
}

void translateVarList(node root) {
    translateParamDec(root->child);
    if(getChildNum(root) > 1)
        translateVarList(getKChild(root, 2));
}

void translateParamDec(node root) {
    translateVarDec_A(getKChild(root, 1));
    return ;
}
// FunDec: VarDec
void translateVarDec_A(node root) {
    node n0 = getKChild(root, 0);
    if(strcmp(n0->name, "ID") == 0) {
        Symbol findTuple = findSymbol(n0->val);
        Operand op_tmp = (Operand)malloc(sizeof(struct Operand_));
        op_tmp->kind = VARIABLE;
        strcpy(op_tmp->u.value, n0->val);
        InterCode code = createCode();
        code->kind = MYPARAM;
        code->u.op_single.op = op_tmp;
        insertCode(code);
        if(findTuple->type->kind == ARRAY) {
            //TODO1: mutil-dimensional array(should pass, because params only have 1d-array)
            //TODO2: params of 1d-array 
        }
        else if(findTuple->type->kind == STRUCTURE || findTuple->type->kind == STRUCTVAR) {
            printf("Cannot translate: Code contains variables or paraneters of structure type\n");
        }
    }
}

//Local Variable: VarDec
void translateVarDec_B(node root) {
    node n0 = getKChild(root, 0);
    if(strcmp(n0->name, "ID") == 0) {
        Symbol findTuple = findSymbol(n0->val);
        if(findTuple->type->kind == STRUCTURE || findTuple->type->kind == STRUCTVAR) {
            printf("Cannot translate: Code contains variables or paraneters of structure type\n");
        }
        else if(findTuple->type->kind == ARRAY) {
            //REVISE:delete multi-dimensional array process
            int size = calculateSize(findTuple->type);
            Operand op_tmp = (Operand)malloc(sizeof(struct Operand_));
            op_tmp->kind = VARIABLE;
            strcpy(op_tmp->u.value, n0->val);
            InterCode code = createCode();
            code->kind = MYDEC;
            code->u.op_dec.op = op_tmp;
            code->u.op_dec.size = size;
            insertCode(code);
        }
    }
    else 
        translateVarDec_B(n0);
}

void translateCompst(node root) {
    node n1 = getKChild(root, 1); 
    if(strcmp(n1->name,"DefList")==0)
    {
        translateDefList_A(n1);
        n1=n1->sibling;
    }
    if(strcmp(n1->name,"StmtList")==0)
        translateStmtList(n1);
}
// DefList in Function
void translateDefList_A(node root) {
    if(root != NULL) {
        translateDef_A(getKChild(root, 0));
        translateDefList_A(getKChild(root, 1));
    }
}

void translateDef_A(node root) {
    translateDecList_A(getKChild(root, 1));
}

void translateDecList_A(node root) {
    if(getChildNum(root) == 1) 
        translateDec_A(getKChild(root, 0));
    else {
        translateDec_A(getKChild(root, 0));
        translateDecList_A(getKChild(root, 2));
    }
}

void translateDec_A(node root) {
    if(getChildNum(root) == 1)
        translateVarDec_B(getKChild(root, 0));
    else {
        Operand op_left = (Operand)malloc(sizeof(struct Operand_));
        op_left->kind = VARIABLE;
        strcpy(op_left->u.value, root->child->child->val);
        Operand op_right = createOpTmp();
        translateExp(getKChild(root, 2), op_right);
        InterCode code = createCode();
        code->kind = MYASSIGN;
        code->u.op_assign.left = op_left;
        code->u.op_assign.right = op_right;
        insertCode(code);
        if(op_right->kind == CONSTANT || op_right->kind == COSNTVAR) {
            // TODO
        }
    }
    
}

void translateStmtList(node root) {
    if(root != NULL) {
        translateStmt(getKChild(root, 0));
        translateStmtList(getKChild(root, 1));
    }
}

void translateStmt(node root) {
    node n0 = getKChild(root, 0);
    if(strcmp(n0->name, "Exp") == 0){
        Operand op = (Operand)malloc(sizeof(struct Operand_));
        op->kind = NOTHING;
        translateExp(n0, op);
    }
    else if(strcmp(n0->name, "Compst") == 0)
        translateCompst(n0);
    else if(strcmp(n0->name, "RETURN") == 0) {
        Operand op = createOpTmp();
        node n1 = getKChild(root, 1);
        translateExp(n1, op);
        InterCode code = createCode();
        code->kind = MYRETURN;
        code->u.op_single.op = op;
        insertCode(code);
    }
    else if(strcmp(n0->name, "IF") == 0) {
        // IF LP EXP RP STMT
        if(getChildNum(root) == 5) {
            Operand op_label1 = createOpLabel();
            int label1 = labelNum;
            Operand op_label2 = createOpLabel();
            int label2 = labelNum;
            
            node n2 = getKChild(root, 2);
            translateCond(n2, label1, label2);

            InterCode code1 = createCode();
            code1->kind = MYLABEL;
            code1->u.op_single.op = op_label1;
            insertCode(code1);

            //increase cnt2
            node n4 = getKChild(root, 4);
            translateStmt(n4);
            //decrease cnt2

            InterCode code2 = createCode();
            code2->kind = MYLABEL;
            code2->u.op_single.op = op_label2;
            insertCode(code2);
        }
        // IF LP EXP RP STMT ELSE STMT
        else {
            Operand op_label1 = createOpLabel();
            int label1 = labelNum;
            Operand op_label2 = createOpLabel();
            int label2 = labelNum;
            Operand op_label3 = createOpLabel();
            int label3 = labelNum;
            
            node n2 = getKChild(root, 2);
            translateCond(n2, label1, label2);

            InterCode code1 = createCode();
            code1->kind = MYLABEL;
            code1->u.op_single.op = op_label1;
            insertCode(code1);

            //increase cnt2
            node n4 = getKChild(root, 4);
            translateStmt(n4);
            //decrease cnt2

            InterCode code2 = createCode();
            code2->kind = MYGOTO;
            Operand op_tmp3 = copyOpLabel(label3);
            code2->u.op_single.op = op_tmp3;
            insertCode(code2);

            InterCode code3 = createCode();
            code3->kind = MYLABEL;
            code3->u.op_single.op = op_label2;
            insertCode(code3);

            //increase cnt2
            node n6 = getKChild(root, 6);
            translateStmt(n6);
            //decrease cnt2

            InterCode code4 = createCode();
            code4->kind = MYLABEL;
            code4->u.op_single.op = op_label3;
            insertCode(code4);
        }
    }
    // WHILE LP EXP RP STMT (unfinished)
    else {
        Operand op_label1 = createOpLabel();
        int label1 = labelNum;
        Operand op_label2 = createOpLabel();
        int label2 = labelNum;
        Operand op_label3 = createOpLabel();
        int label3 = labelNum;

        InterCode code1 = createCode();
        code1->kind = MYLABEL;
        code1->u.op_single.op = op_label1;
        insertCode(code1);

        //set cnt = 1
        node n2 = getKChild(root, 2);
        translateCond(n2, label2, label3);
        //set cnt = 0

        InterCode code2 = createCode();
        code2->kind = MYLABEL;
        code2->u.op_single.op = op_label2;
        insertCode(code2);

        // cnt1 ++;
        node n4 = getKChild(root, 4);
        translateStmt(n4);
        // cnt1 --;

        InterCode code3 = createCode();
        code3->kind = MYGOTO;
        Operand op_tmp1 = copyOpLabel(label1);
        code3->u.op_single.op = op_tmp1;
        insertCode(code3);

        InterCode code4 = createCode();
        code4->kind = MYLABEL;
        code4->u.op_single.op = op_label3;
        insertCode(code4);
    }
}

void translateExp(node root, Operand op) {
    node n0 = getKChild(root, 0);
    node n1 = getKChild(root, 1);
    node n2 = getKChild(root, 2);
    int childNum = getChildNum(root);
    if(childNum == 1) {
        if(strcmp(n0->name, "ID") == 0) {
            Symbol findTuple = findSymbol(n0->val); 
            op->kind = VARIABLE;
            strcpy(op->u.value, n0->val);
        }
        else if(strcmp(n0->name, "INT") == 0) {
            op->kind = CONSTANT;
            op->u.var_no = atoi(n0->val);
        }
    } 
    else if(childNum == 2) {
        if(strcmp(n0->name, "MINUS") == 0) {
            Operand op_tmp = createOpTmp();
            translateExp(n1, op_tmp);
            if(op_tmp->kind == CONSTANT || op_tmp->kind == COSNTVAR) {
                op->kind = COSNTVAR;
                op->u.var_no = op_tmp->u.var_no * (-1);
            }
            else {
                Operand op_const = (Operand)malloc(sizeof(struct Operand_));
                op_const->kind = CONSTANT;
                op_const->u.var_no = 0;

                InterCode code2 = createCode();
                code2->kind = MYSUB;
                code2->u.op_binary.op1 = op_const;
                code2->u.op_binary.op2 = op_tmp;
                code2->u.op_binary.result = op;
                insertCode(code2);
            }
        }
        else if(strcmp(n0->name, "NOT") == 0) {
            translateExpCommon(root, op);
        }
    }
    else if(childNum == 3) {
        if(strcmp(n0->name, "LP") == 0) 
            translateExp(n1, op);
        else if(strcmp(n1->name, "ASSIGNOP") == 0) {
            // ALL kinds of ASSIGNOP
            Operand op_tmp = createOpTmp();
            translateExp(n0, op_tmp);
            if(op_tmp->kind == COSNTVAR) {
                op_tmp->kind = VARIABLE;
                strcpy(op_tmp->u.value, n0->child->val);
            }
            Operand t1 = createOpTmp();
            translateExp(n2, t1);
            InterCode code1 = createCode();
            code1->kind = MYASSIGN;
            code1->u.op_assign.left = op_tmp;
            code1->u.op_assign.right = t1;
            insertCode(code1);
            // place := varialbe.name
            if(op->kind != NOTHING) {
                op->kind = op_tmp->kind;
                strcpy(op->u.value, op_tmp->u.value);
            }
        }
        else if(strcmp(n1->name, "AND") == 0 || strcmp(n1->name, "OR") == 0 || strcmp(n1->name, "RELOP") == 0) {
            translateExpCommon(root, op);
        }
        else if(strcmp(n1->name, "DOT") == 0) {
            printf("cannot translate struct\n");
        }
        else if(strcmp(n0->name, "ID") == 0) {
            translateExpFunc(root, op);
        }
        else {
            translateExpMath(root, op);
        }
    }
}

void translateExpCommon(node root, Operand place) {
    Operand op_label1 = createOpLabel();
    int label1 = labelNum;
    Operand op_label2 = createOpLabel();
    int label2 = labelNum;
    Operand const0 = (Operand)malloc(sizeof(struct Operand_));;
    const0->kind = CONSTANT;
    const0->u.var_no = 0;
    Operand const1 = (Operand)malloc(sizeof(struct Operand_));;
    const1->kind = CONSTANT;
    const1->u.var_no = 1;

    InterCode code0 = createCode();
    code0->kind = MYASSIGN;
    code0->u.op_assign.left = place;
    code0->u.op_assign.right = const0;
    insertCode(code0);

    translateCond(root, label1, label2);
    
    InterCode code1 = createCode();
    code1->kind = MYLABEL;
    code1->u.op_single.op = op_label1;
    insertCode(code1);

    InterCode code2 = createCode();
    code2->kind = MYASSIGN;
    code2->u.op_assign.left = place;
    code2->u.op_assign.right = const1;
    insertCode(code2);
    
    InterCode code3 = createCode();
    code3->kind = MYLABEL;
    code3->u.op_single.op = op_label2;
    insertCode(code3);
}

void translateExpFunc(node root, Operand place) {

}

void translateExpMath(node root, Operand place) {
    node n0 = getKChild(root, 0);
    node n1 = getKChild(root, 1);
    node n2 = getKChild(root, 2);
    Operand t1 = createOpTmp();
    translateExp(n0, t1);
    Operand t2 = createOpTmp();
    translateExp(n2, t2);
    if((t1->kind == CONSTANT || t1->kind == COSNTVAR) && (t2->kind == CONSTANT || t2->kind == COSNTVAR)) {
        place->kind = COSNTVAR;
        if(strcmp(n1->name, "PLUS") == 0) 
            place->u.var_no = t1->u.var_no + t2->u.var_no;
        else if(strcmp(n1->name, "MINUS") == 0) 
            place->u.var_no = t1->u.var_no - t2->u.var_no;
        else if(strcmp(n1->name, "STAR") == 0) 
            place->u.var_no = t1->u.var_no * t2->u.var_no;
        else if(strcmp(n1->name, "DIV") == 0) 
            place->u.var_no = t1->u.var_no / t2->u.var_no;
    }
    else {
        InterCode code2 = createCode();
        //REVISE:ADD code2->u.op_binary._operator
        if(strcmp(n1->name, "PLUS") == 0) 
        {
            code2->kind = MYADD;
            code2->u.op_binary._operator='+';
        }
        else if(strcmp(n1->name, "MINUS") == 0) 
        {
            code2->kind = MYSUB;
            code2->u.op_binary._operator='-';
        }
        else if(strcmp(n1->name, "STAR") == 0)
        { 
            code2->kind = MYMUL;
            code2->u.op_binary._operator='*';
        }
        else if(strcmp(n1->name, "DIV") == 0) 
        {
            code2->kind = MYDIV;
            code2->u.op_binary._operator='/';
        }
        code2->u.op_binary.op1 = t1;
        code2->u.op_binary.op2 = t2;
        code2->u.op_binary.result = place;
        insertCode(code2);
    }
}

void translateCond(node root, int label_true, int label_false) {
    node n1 = getKChild(root ,1);
    node n0 = getKChild(root, 0);
    node n2 = getKChild(root, 2);
    if(n1 && strcmp(n1->name, "RELOP") == 0) {
        Operand t1 = createOpTmp();
        Operand t2 = createOpTmp();

        translateExp(n0, t1);
        translateExp(n2, t2);

        InterCode code3 = createCode();
        code3->kind = MYIFGOTO;
        code3->u.op_triple.x = t1;
        code3->u.op_triple.y = t2;
        strcpy(code3->u.op_triple.relop, n1->val);
        code3->u.op_triple.label = copyOpLabel(label_true);
        insertCode(code3);

        InterCode code4 = createCode();
        code4->kind = MYGOTO;
        code4->u.op_single.op = copyOpLabel(label_false);
        insertCode(code4);
    }
    else if(n0 && strcmp(n0->name, "NOT") == 0) 
        translateCond(n1, label_false, label_true);
    else if(n1 && strcmp(n1->name, "AND") == 0) {
        Operand op_label1 = createOpLabel();
        int label1 = labelNum;

        translateCond(n0, label1, label_false);

        InterCode code1 = createCode();
        code1->kind = MYLABEL;
        code1->u.op_single.op = op_label1;
        insertCode(code1);

        translateCond(n2, label_true, label_false);
    }
    else if(n1 && strcmp(n1->name, "OR") == 0) {
        Operand op_label1 = createOpLabel();
        int label1 = labelNum;

        translateCond(n0, label_true, label1);

        InterCode code1 = createCode();
        code1->kind = MYLABEL;
        code1->u.op_single.op = op_label1;
        insertCode(code1);

        translateCond(n2, label_true, label_false);
    }
    else {
        Operand t1 = createOpTmp();
        translateExp(root, t1);
        Operand t2 = (Operand)malloc(sizeof(struct Operand_));
        t2->kind = CONSTANT;
        t2->u.var_no = 0;

        InterCode code2 = createCode();
        code2->kind = MYIFGOTO;
        code2->u.op_triple.x = t1;
        code2->u.op_triple.y = t2;
        code2->u.op_triple.label = copyOpLabel(label_true);
        strcpy(code2->u.op_triple.relop, "!=");
        insertCode(code2);

        InterCode code3 = createCode();
        code3->kind = MYGOTO;
        code3->u.op_single.op = copyOpLabel(label_false);
        insertCode(code3);
    }
}