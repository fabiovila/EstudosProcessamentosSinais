w = 0.2
phi = 0
a = 2 * cos(w);
samples = 100
y = zeros(1, samples);
y(1) = sin(phi);
y(2) = sin(w + phi);

% Calcule os valores de y para i de 3 a N
for i = 3:samples
    y(i) = a * y(i - 1) - y(i - 2);
end

% Agora, o vetor 'y' conterá os valores desejados

stem(y);
