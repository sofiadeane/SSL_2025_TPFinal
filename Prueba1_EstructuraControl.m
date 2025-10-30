inicio
    X := 5;
    R := 2.0;
    C := 'C';

    si (X > 0)
        escribir(C);

    mientras (X >= 0)
        X := X - 1;

    repetir
        C := 'Z';
        escribir(C);
        R := R - 0.5;
    hasta (R <= 0.0);

fin
