FileName = 'testcase_alazar.data';
nScans = 640;
nAlines = 30;

% %% Read background data
% fid = fopen(BackgroundFileName, 'r');
% bg = mean(fread(fid, [nScans nAlines], 'uint16'), 2);
% fclose(fid);

%% Read signal data
fid = fopen(FileName, 'r');
data = fread(fid, [nScans nAlines], 'uint16');
fclose(fid);

%% Background signal by averaging a-lines
bg = mean(data, 2);
% plot(bg);

%% Subtract bg
data = data - repmat(bg, 1, nAlines);

figure(1);
plot(data);
title('Signal Data');

%% Intensity value
intensity = abs(ifft(data, [], 1)).^2;
intensity = intensity(1:size(intensity, 1)/2, :);

%% Normalize intensity value to make image
image = -10 * log10(intensity / 5e13);
image = (image - min(min(image))) / (max(max(image)) - min(min(image)));

figure(2);
plot(image);
title('Image Data');

%% Save image
imwrite(image, 'result.bmp');