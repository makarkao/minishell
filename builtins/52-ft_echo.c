/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   52-ft_echo.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: melayyad <melayyad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/22 17:29:05 by melayyad          #+#    #+#             */
/*   Updated: 2025/07/13 21:58:59 by melayyad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../minishell.h"

int	is_echo_n_option(const char *arg)
{
	int	i;

	i = 1;
	if (arg[0] != '-' || arg[1] == '\0')
		return (0);
	while (arg[i])
	{
		if (arg[i] != 'n')
			return (0);
		i++;
	}
	return (1);
}

int	ft_echo(char **str)
{
	int	i;
	int	print_newline;

	i = 0;
	print_newline = 1;
	str++;
	while (str[i] && is_echo_n_option(str[i]))
	{
		print_newline = 0;
		i++;
	}
	while (str[i])
	{
		if (ft_putstr_fd(1, str[i]) < 0)
			return (1);
		if (str[i + 1])
			ft_putstr_fd(1, " ");
		i++;
	}
	if (print_newline)
		write(1, "\n", 1);
	return (0);
}
