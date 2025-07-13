/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   55-ft_pwd.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: melayyad <melayyad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/22 23:00:21 by melayyad          #+#    #+#             */
/*   Updated: 2025/07/13 21:59:05 by melayyad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../minishell.h"

int	ft_pwd(t_shelldata *shelldata)
{
	char	*value;

	if (shelldata->cwd)
		return (ft_putstr_fd(1, shelldata->cwd), write(1, "\n", 1), 0);
	value = getcwd(NULL, 0);
	if (value != NULL)
	{
		ft_putstr_fd(1, value);
		free(value);
		write(1, "\n", 1);
		return (0);
	}
	perror("pwd: error retrieving current directory: getcwd:"
		" cannot access parent directories: ");
	return (1);
}
