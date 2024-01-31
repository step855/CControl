/*
 * violajones.c
 *
 *  Created on: 27 Januari 2024
 *      Author: Daniel Mårtensson
 */

#include "computervision.h"

/*
 * Collect the data for the Viola Jones algorithm
 * data[total_data_rows * (row * column)] - Output
 * y[total_data_rows] - Output
 */
void voilajones_collect(uint32_t* data[], int8_t* y[], size_t* total_data_rows, const char train_path_objects[], const char train_path_non_objects[], const size_t row, const size_t column) {
	/* Scan all objects */
	char** file_names_objects = NULL;
	const size_t total_objects = scan_file_names(train_path_objects, &file_names_objects, ".pgm");

	/* Scan all non-objects */
	char** file_names_non_objects = NULL;
	const size_t total_non_objects = scan_file_names(train_path_non_objects, &file_names_non_objects, ".pgm");

	/* Compute total_data_rows */
	*total_data_rows = total_objects + total_non_objects;

	/* Generate integral X matrix */
	const size_t row_column = row * column;
	uint32_t* X = (uint32_t*)malloc(row_column * sizeof(uint32_t));

	/* Create the total path */
	char total_path[260];

	/* Create data */
	*data = (uint32_t*)malloc(*total_data_rows * row_column * sizeof(uint32_t));
	*y = (int8_t*)malloc(*total_data_rows * sizeof(int8_t));

	/* Generate the data for objects */
	size_t i;
	for (i = 0; i < total_objects; i++) {
		/* Combine train path with file name */
		concatenate_paths(total_path, train_path_objects, file_names_objects[i]);

		/* Read the image */
		PGM* image = imread(total_path);

		/* Generate integral image X */
		integralimage(image->pixels, X, row, column);

		/* Copy over in row-major */
		memcpy(*data + i * row_column , X, row_column * sizeof(uint32_t));

		/* Save the label */
		(*y)[i] = 1;
		
		/* Free */
		imfree(image);
	}

	/* Generate for non-objects */
	for (i = 0; i < total_non_objects; i++) {
		/* Combine train path with file name */
		concatenate_paths(total_path, train_path_non_objects, file_names_non_objects[i]);

		/* Read the image */
		PGM* image = imread(total_path);

		/* Generate integral image X */
		integralimage(image->pixels, X, row, column);

		/* Copy over in row-major */
		memcpy(*data + i * row_column + total_objects * row_column, X, row_column * sizeof(uint32_t));

		/* Save the label */
		(*y)[i + total_objects] = -1.0f;

		/* Free */
		imfree(image);
	}

	/* Free */
	for (i = 0; i < total_objects; i++) {
		free(file_names_objects[i]);
	}
	free(file_names_objects);
	for (i = 0; i < total_non_objects; i++) {
		free(file_names_non_objects[i]);
	}
	free(file_names_non_objects);
	free(X);
}

/* 
 * Viola Jones algorithm for traning
 * X[total_train_data_rows * (row*column)] - Input
 * y[total_train_data_rows] - Input
 * N - How many features returns back
 * row - Row size of the image in X
 * column - Column size of the image in X
 */
VIOLAJONES_MODEL* violajones_train(const HAARLIKE_FEATURE best_features[], const uint32_t X[], const int8_t y[], const size_t total_train_data_rows, const size_t total_haarlikes, const uint8_t N, const uint8_t row, const uint8_t column) {
	/* Generate the Haar-like features */
	HAARLIKE_FEATURE* features = haarlike_features(total_haarlikes - N, row, column);

	/* Add in the old best features as a feedback at the bottom */
	features = realloc(features, total_haarlikes * sizeof(HAARLIKE_FEATURE));
	size_t i;
	for (i = total_haarlikes - N; i < total_haarlikes; i++) {
		features[i] = best_features[i - (total_haarlikes - N)];
	}

	/* Create adaboost data */
	float* data = (float*)malloc(total_train_data_rows * total_haarlikes * sizeof(float));
	float* y_float = (float*)malloc(total_train_data_rows * sizeof(float));

	/* Generate the data for objects and non-objects */
	size_t j;
	const size_t row_column = row * column;
	for (i = 0; i < total_train_data_rows; i++) {
		/* Collect the data from haar-like features */
		for (j = 0; j < total_haarlikes; j++) {
			features[j].value = haarlike_predict(X + i * row_column, &features[j], row, column);
			data[i * total_haarlikes + j] = (float)features[j].value;
		}
	}

	/* Shuffle the data */
	size_t* index = (size_t*)malloc(total_train_data_rows * sizeof(size_t));
	shuffle(data, index, true, total_train_data_rows, total_haarlikes);
	for (i = 0; i < total_train_data_rows; i++) {
		y_float[i] = (float)y[index[i]];
	}

	/* Create AdaBoost model */
	ADABOOST_MODEL* model_adaboost = adaboost_train(data, y_float, N, total_train_data_rows, total_haarlikes);

	/* Check which feature indexes of the model that we are going to save */
	VIOLAJONES_MODEL* model = (VIOLAJONES_MODEL*)malloc(N * sizeof(VIOLAJONES_MODEL));
	for (i = 0; i < N; i++) {
		model[i].adaboost_model = model_adaboost[i];
		model[i].haarlike_feature = features[model_adaboost[i].feature_index];

		/* It's important to send the feature index to i for the adaboost prediction */
		model[i].adaboost_model.feature_index = i;
	}

	/* Free */
	free(data);
	free(y_float);
	free(index);
	free(model_adaboost);
	free(features);

	/* Return model */
	return model;
}

/*
 * Evaluate the Haar-Like features on a test data set 
 * features[N]
 * row - Row size of the image in X
 * column - Column size of the image in X
 */
float violajones_eval(const VIOLAJONES_MODEL models[], const size_t N, const uint32_t X[], const int8_t y[], const size_t total_test_data_rows, const uint8_t row, const uint8_t column) {
	/* Bring adaboost model */
	ADABOOST_MODEL* adaboost_models = (ADABOOST_MODEL*)malloc(N * sizeof(ADABOOST_MODEL));
	size_t i;
	for (i = 0; i < N; i++) {
		adaboost_models[i] = models[i].adaboost_model;
	}

	/* Create x vector */
	float* x = (float*)malloc(N * sizeof(float));
	
	/* Generate the data for faces */
	size_t j;
	float object_found = 0.0f;
	float non_object_found = 0.0f;
	float total_objects = 0.0f;
	float total_non_objects = 0.0f;
	const size_t row_column = row * column;
	for (i = 0; i < total_test_data_rows; i++) {
		size_t count = 0;
		/* Do Collect data */
		for (j = 0; j < N; j++) {
			x[j] = (float)haarlike_predict(X + i * row_column, &models[j].haarlike_feature, row, column);
		}

		/* Check what we are going to count */
		if(y[i] == 1){
			/* Check if it's was true */
			if (adaboost_predict(adaboost_models, x, N) == 1) {
				object_found++;
			}

			/* Count the total objects */
			total_objects++;
		}
		else {
			/* Check if it's was true */
			if (adaboost_predict(adaboost_models, x, N, N) == -1) {
				non_object_found++;
			}

			/* Count the total non objects */
			total_non_objects++;
		}
	}

	/* Free */
	free(adaboost_models);
	free(x);

	/* Compute the accuracy */
	return object_found / total_objects * non_object_found / total_non_objects * 100.0f;
}